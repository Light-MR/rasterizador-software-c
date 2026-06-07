#!/usr/bin/env python3
"""
gltf2tdm.py — Convierte GLTF + .bin a formato TDM para el pipeline gs.c

Uso:
    python gltf2tdm.py <ruta/scene.gltf> -o <salida.tdm> [--tex]

Dependencia (solo --tex): pip install Pillow
"""

import json, struct, sys, math, argparse
from pathlib import Path

# ── Tablas GLTF ───────────────────────────────────────────────────────────────
COMP_FMT  = {5120:'b', 5121:'B', 5122:'h', 5123:'H', 5125:'I', 5126:'f'}
COMP_SIZE = {5120:1,   5121:1,   5122:2,   5123:2,   5125:4,   5126:4}
TYPE_DIM  = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT2':4,'MAT3':9,'MAT4':16}

# ── Leer un accessor del buffer binario ───────────────────────────────────────
def read_accessor(gltf, bin_data, idx):
    acc  = gltf['accessors'][idx]
    bv   = gltf['bufferViews'][acc['bufferView']]
    fmt  = COMP_FMT[acc['componentType']]
    csz  = COMP_SIZE[acc['componentType']]
    dim  = TYPE_DIM[acc['type']]
    cnt  = acc['count']
    base = bv.get('byteOffset', 0) + acc.get('byteOffset', 0)
    step = bv.get('byteStride', csz * dim)
    out  = []
    for i in range(cnt):
        vals = struct.unpack_from(f'<{dim}{fmt}', bin_data, base + i * step)
        out.append(vals)
    return out

# ── Matrices 4×4 (column-major, igual que GLTF y OpenGL) ─────────────────────
def m4_id():
    m = [0.0] * 16
    m[0] = m[5] = m[10] = m[15] = 1.0
    return m

def m4_mul(A, B):
    R = [0.0] * 16
    for c in range(4):
        for r in range(4):
            R[c*4+r] = sum(A[k*4+r] * B[c*4+k] for k in range(4))
    return R

def xf_pos(m, p):
    """Transforma un punto 3D (w=1) con una matriz 4×4 column-major."""
    x, y, z = p
    rx = m[0]*x + m[4]*y + m[8]*z  + m[12]
    ry = m[1]*x + m[5]*y + m[9]*z  + m[13]
    rz = m[2]*x + m[6]*y + m[10]*z + m[14]
    w  = m[3]*x + m[7]*y + m[11]*z + m[15]
    if abs(w - 1.0) > 1e-6:
        rx /= w; ry /= w; rz /= w
    return (rx, ry, rz)

def xf_dir(m, n):
    """Transforma una dirección 3D (w=0) — solo parte 3×3 de rotación."""
    x, y, z = n
    return (m[0]*x + m[4]*y + m[8]*z,
            m[1]*x + m[5]*y + m[9]*z,
            m[2]*x + m[6]*y + m[10]*z)

def xf_normal(m, n):
    """Transforma una normal con la inversa-transpuesta de M3×3.
    Necesario cuando M tiene escala no-uniforme; para escala uniforme
    o pura rotación el resultado es equivalente a xf_dir."""
    a, b, c = m[0], m[1], m[2]   # columna 0
    d, e, f = m[4], m[5], m[6]   # columna 1
    g, h, i = m[8], m[9], m[10]  # columna 2
    det = a*(e*i - f*h) - d*(b*i - c*h) + g*(b*f - c*e)
    if abs(det) < 1e-10:
        return xf_dir(m, n)       # fallback: matriz degenerada
    inv = 1.0 / det
    nx, ny, nz = n
    # Multiplica por adj(M) (= cof(M)^T) / det — transforma normales correctamente
    rx = ((e*i - f*h)*nx + (f*g - d*i)*ny + (d*h - e*g)*nz) * inv
    ry = ((c*h - b*i)*nx + (a*i - c*g)*ny + (b*g - a*h)*nz) * inv
    rz = ((b*f - c*e)*nx + (c*d - a*f)*ny + (a*e - b*d)*nz) * inv
    return (rx, ry, rz)

def norm3(v):
    x, y, z = v
    l = math.sqrt(x*x + y*y + z*z)
    return (x/l, y/l, z/l) if l > 1e-8 else (0., 1., 0.)

def node_local_mat(node):
    """Devuelve la matriz local del nodo (column-major, 16 floats)."""
    if 'matrix' in node:
        return list(map(float, node['matrix']))
    T = m4_id(); R = m4_id(); S = m4_id()
    if 'translation' in node:
        t = node['translation']
        T[12] = t[0]; T[13] = t[1]; T[14] = t[2]
    if 'rotation' in node:
        x, y, z, w = node['rotation']
        R[0]  = 1-2*(y*y+z*z); R[4] = 2*(x*y-w*z); R[8]  = 2*(x*z+w*y)
        R[1]  = 2*(x*y+w*z);   R[5] = 1-2*(x*x+z*z); R[9] = 2*(y*z-w*x)
        R[2]  = 2*(x*z-w*y);   R[6] = 2*(y*z+w*x); R[10] = 1-2*(x*x+y*y)
    if 'scale' in node:
        s = node['scale']
        S[0] = s[0]; S[5] = s[1]; S[10] = s[2]
    return m4_mul(T, m4_mul(R, S))

# ── Recorrido del árbol de nodos ──────────────────────────────────────────────
def walk_nodes(gltf, node_idx, parent_mat, results):
    """Acumula (mesh_idx, world_matrix) para cada nodo con mesh."""
    node  = gltf['nodes'][node_idx]
    world = m4_mul(parent_mat, node_local_mat(node))
    if 'mesh' in node:
        results.append((node['mesh'], world))
    for child in node.get('children', []):
        walk_nodes(gltf, child, world, results)

# ── Cálculo de normales por cara (fallback si el modelo no tiene normales) ────
def compute_normals(positions, indices):
    acc = [[0., 0., 0.] for _ in positions]
    for t in range(0, len(indices), 3):
        i0, i1, i2 = indices[t], indices[t+1], indices[t+2]
        p0, p1, p2 = positions[i0], positions[i1], positions[i2]
        ax = p1[0]-p0[0]; ay = p1[1]-p0[1]; az = p1[2]-p0[2]
        bx = p2[0]-p0[0]; by = p2[1]-p0[1]; bz = p2[2]-p0[2]
        cx = ay*bz - az*by
        cy = az*bx - ax*bz
        cz = ax*by - ay*bx
        for i in (i0, i1, i2):
            acc[i][0] += cx; acc[i][1] += cy; acc[i][2] += cz
    return [norm3(tuple(n)) for n in acc]

# ── Encontrar URI de textura diffuse/baseColor ────────────────────────────────
def find_diffuse_uri(gltf):
    for mat in gltf.get('materials', []):
        pbr = mat.get('pbrMetallicRoughness', {})
        if 'baseColorTexture' in pbr:
            ti = pbr['baseColorTexture']['index']
            si = gltf['textures'][ti]['source']
            return gltf['images'][si]['uri']
        ext = mat.get('extensions', {}).get('KHR_materials_pbrSpecularGlossiness', {})
        if 'diffuseTexture' in ext:
            ti = ext['diffuseTexture']['index']
            si = gltf['textures'][ti]['source']
            return gltf['images'][si]['uri']
    return None

# ── Convertir PNG/JPG → BMP 24bpp BGR bottom-up ───────────────────────────────
def convert_texture(src_path, dst_path):
    try:
        from PIL import Image
    except ImportError:
        sys.exit("Instala Pillow para convertir texturas:  pip install Pillow")
    img = Image.open(src_path).convert('RGB')
    img = img.transpose(Image.FLIP_TOP_BOTTOM)   # row 0 = abajo (convención del pipeline)
    img.save(str(dst_path), format='BMP')
    print(f"  Textura BMP: {dst_path.name}  ({dst_path.stat().st_size // 1024} KB)")

# ── Conversión principal ───────────────────────────────────────────────────────
def convert(gltf_path, out_path, extract_tex):
    gltf_path = Path(gltf_path)
    out_path  = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    gltf     = json.loads(gltf_path.read_text(encoding='utf-8'))
    buf_uri  = gltf['buffers'][0]['uri']
    bin_data = (gltf_path.parent / buf_uri).read_bytes()
    print(f"Leyendo: {gltf_path.name}  (bin {len(bin_data)//1024} KB)")

    # Recopilar meshes con sus matrices de mundo
    mesh_entries = []
    scene_idx    = gltf.get('scene', 0)
    for root in gltf['scenes'][scene_idx]['nodes']:
        walk_nodes(gltf, root, m4_id(), mesh_entries)

    if not mesh_entries:
        sys.exit("Error: no se encontraron meshes en la escena GLTF")

    all_pos  = []; all_norm = []; all_uv = []
    all_col  = []; all_idx  = []
    vert_offset = 0

    for mesh_idx, world in mesh_entries:
        mesh = gltf['meshes'][mesh_idx]
        print(f"  Mesh: '{mesh['name']}'")

        for prim in mesh['primitives']:
            mode = prim.get('mode', 4)
            if mode != 4:
                print(f"    Saltando primitiva (mode={mode}, solo TRIANGLES=4)")
                continue

            attrs = prim['attributes']

            # ── Posiciones ──
            pos_list = [xf_pos(world, p)
                        for p in read_accessor(gltf, bin_data, attrs['POSITION'])]
            nv = len(pos_list)

            # ── Índices ──
            if 'indices' in prim:
                raw_idx = [r[0] for r in read_accessor(gltf, bin_data, prim['indices'])]
            else:
                raw_idx = list(range(nv))  # sin índices → secuencial

            # ── Normales (inversa-transpuesta para escala no-uniforme) ──
            if 'NORMAL' in attrs:
                norm_list = [norm3(xf_normal(world, n))
                             for n in read_accessor(gltf, bin_data, attrs['NORMAL'])]
            else:
                norm_list = compute_normals(pos_list, raw_idx)

            # ── UVs (flip V: GLTF v=0 arriba, pipeline v=0 abajo) ──
            if 'TEXCOORD_0' in attrs:
                uv_list = [(u, 1.0 - v)
                           for u, v in read_accessor(gltf, bin_data, attrs['TEXCOORD_0'])]
            else:
                uv_list = [(0.0, 0.0)] * nv

            # ── Colores de vértice ──
            if 'COLOR_0' in attrs:
                acc_col   = gltf['accessors'][attrs['COLOR_0']]
                is_float  = (acc_col['componentType'] == 5126)
                col_raw   = read_accessor(gltf, bin_data, attrs['COLOR_0'])
                col_list  = []
                for c in col_raw:
                    if is_float:
                        r = int(min(1., max(0., c[0])) * 255)
                        g = int(min(1., max(0., c[1])) * 255)
                        b = int(min(1., max(0., c[2])) * 255)
                        a = int(min(1., max(0., c[3])) * 255) if len(c) > 3 else 255
                    else:
                        r, g, b = int(c[0]), int(c[1]), int(c[2])
                        a = int(c[3]) if len(c) > 3 else 255
                    col_list.append((r, g, b, a))
            else:
                col_list = [(255, 255, 255, 255)] * nv

            all_pos.extend(pos_list)
            all_norm.extend(norm_list)
            all_uv.extend(uv_list)
            all_col.extend(col_list)
            all_idx.extend(i + vert_offset for i in raw_idx)
            vert_offset += nv

    n_verts = len(all_pos)
    n_index = len(all_idx)
    print(f"  Total: {n_verts} vértices, {n_index // 3} triángulos")

    # ── Escribir TDM ──
    with open(out_path, 'wb') as f:
        f.write(b'TDMD')
        f.write(struct.pack('<III', 1, n_verts, n_index))
        for i in range(n_verts):
            px, py, pz = all_pos[i]
            nx, ny, nz = all_norm[i]
            u,  v      = all_uv[i]
            r,  g, b, a = all_col[i]
            f.write(struct.pack('<ffffffffBBBB',
                                px, py, pz,
                                nx, ny, nz,
                                u,  v,
                                r,  g, b, a))
        for idx in all_idx:
            f.write(struct.pack('<I', idx))

    print(f"Escrito: {out_path}  ({out_path.stat().st_size // 1024} KB)")

    # ── Textura ──
    if extract_tex:
        uri = find_diffuse_uri(gltf)
        if uri:
            src = gltf_path.parent / uri
            dst = out_path.with_suffix('.bmp')
            convert_texture(src, dst)
        else:
            print("  (no hay textura diffuse/baseColor en el material)")

# ── Punto de entrada ──────────────────────────────────────────────────────────
if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='Convierte GLTF → TDM')
    ap.add_argument('gltf',          help='Ruta a scene.gltf')
    ap.add_argument('-o', '--output', required=True, help='Ruta de salida .tdm')
    ap.add_argument('--tex',         action='store_true',
                    help='Extraer y convertir textura diffuse/baseColor a .bmp')
    args = ap.parse_args()
    convert(args.gltf, args.output, args.tex)
