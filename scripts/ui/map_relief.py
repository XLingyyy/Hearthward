"""Cartographic relief from measured heights; plan coordinates never move."""
import numpy as np
from PIL import Image, ImageDraw


EXAGGERATION = 4.5


def relief_shading(heights, small, metres_per_pixel, sample_spacing=2):
    dy, dx = np.gradient(heights, metres_per_pixel)
    gx, gy = dx * EXAGGERATION, dy * EXAGGERATION
    light = (.58 * gx + .43 * gy + .69) / np.sqrt(1 + gx * gx + gy * gy)
    direct = np.clip(.20 + 1.08 * light, .26, 1.26)
    # Higher neighbouring terrain occludes the valley floor; exposed crests stay bright.
    horizon = np.zeros_like(small)
    n = small.shape[0]
    for vx, vy in [(-1,-1),(0,-1),(1,-1),(-1,0),(1,0),(-1,1),(0,1),(1,1)]:
        obstruction = np.zeros_like(small)
        for step in [2,4,8,16,32,48,64,96,128]:
            ix = np.clip(np.arange(n) + vx * step, 0, n - 1)
            iy = np.clip(np.arange(n) + vy * step, 0, n - 1)
            neighbour = small[np.ix_(iy, ix)]
            distance = sample_spacing * step * np.hypot(vx, vy)
            obstruction = np.maximum(obstruction, (neighbour - small) * EXAGGERATION / distance)
        horizon += np.sin(np.arctan(obstruction)) / 8
    ambient = np.clip(np.asarray(Image.fromarray(horizon).resize(heights.shape[::-1], Image.Resampling.BICUBIC)), 0, 1)
    shade = direct * np.clip(1 - ambient * .75, .48, 1)
    return shade, np.hypot(dx, dy), ambient


def relief_contours(size, small, sample_spacing=2):
    """Fine paired lines model the slope walls and curved crests of the observed surface."""
    image = Image.new('RGBA', (size, size))
    draw = ImageDraw.Draw(image)
    lookup = {1:[(3,0)],2:[(0,1)],3:[(3,1)],4:[(1,2)],5:[(3,0),(1,2)],6:[(0,2)],7:[(3,2)],
              8:[(2,3)],9:[(0,2)],10:[(0,1),(2,3)],11:[(1,2)],12:[(3,1)],13:[(0,1)],14:[(3,0)]}
    edge_vertices = [(0,1),(1,2),(2,3),(3,0)]
    dy, dx = np.gradient(small, sample_spacing)
    factor = size / (small.shape[0] - 1)
    for level in np.arange(np.floor(small.min() / 2) * 2, small.max(), 2):
        pattern = ((small[:-1,:-1] >= level).astype(np.uint8) + 2*(small[:-1,1:] >= level)
                   + 4*(small[1:,1:] >= level) + 8*(small[1:,:-1] >= level))
        for iy, ix in np.argwhere((pattern != 0) & (pattern != 15)):
            slope = np.hypot(dx[iy,ix],dy[iy,ix])
            if slope < .045:
                continue
            corners = np.array([[ix,iy],[ix+1,iy],[ix+1,iy+1],[ix,iy+1]], dtype=float)
            values = [small[iy,ix],small[iy,ix+1],small[iy+1,ix+1],small[iy+1,ix]]
            def point(edge):
                a,b = edge_vertices[edge]
                t = (level-values[a])/(values[b]-values[a])
                return corners[a]*(1-t)*factor + corners[b]*t*factor
            strength = float(np.clip(slope / .35, .25, 1))
            # The uphill rim catches light; a short downhill shadow describes depth.
            normal = np.array([dx[iy,ix],dy[iy,ix]]) / max(slope, .001)
            for a,b in lookup[int(pattern[iy,ix])]:
                p,q = point(a),point(b)
                shadow = normal * -2.8
                highlight = normal * 1.2
                draw.line([tuple(p+shadow),tuple(q+shadow)], fill=(21,26,25,round(58*strength)), width=3)
                draw.line([tuple(p+highlight),tuple(q+highlight)], fill=(172,171,157,round(72*strength)), width=2)
    return image
