# renderer.py
# VPython-based renderer for the FPGA visualizer.

# The renderer accepts uploaded triangle meshes (via `save_shape`) and
# staged instances (via `stage_instance`) then renders them together when
# `render_frame()` is called (usually on FRAME_END). It is intentionally
# lightweight and designed for debugging visual output from the MCU/FPGA stack.

import math
from typing import Iterable

from vpython import box, color, label, rate, scene, triangle, vector, vertex


class Renderer:
    def __init__(self, use_custom_shapes=True, debug=False):
        self.use_custom_shapes = use_custom_shapes
        self.debug = debug
        self.shape_registry = {}  # id -> list of triangles
        self._staged = []
        self._objects = []

    def log(self, *args):
        if self.debug:
            print(*args)

    def create_scene(self):
        scene.title = "MCU -> FPGA Simulator"
        scene.width = 1200
        scene.height = 900
        scene.center = vector(0, 0, 0)
        scene.forward = vector(0, 0, 1)
        scene.up = vector(0, -1, 0)
        scene.autoscale = False
        scene.background = color.gray(0.2)
        # Ensure the scene is visible and camera is placed so origin is visible
        scene.visible = True
        scene.camera.pos = vector(0, 0, -40)
        self.info_label = label(
            pos=vector(-40, 30, 0), text="", height=16, box=False
        )

    def save_shape(self, shape_id: int, triangles: Iterable):
        self.shape_registry[shape_id] = list(triangles)
        # debug log so we can confirm shapes were uploaded
        print(
            f"[RENDERER] Saved shape {shape_id} with {len(triangles)} triangles"
        )
        self.log(f"Saved shape {shape_id} with {len(triangles)} triangles")

    def stage_instance(self, shape_id: int, pos, rot, is_last=False):
        self._staged.append(
            {"id": shape_id, "pos": pos, "rot": rot, "last": is_last}
        )

    def clear_staging(self):
        """Clear any staged instances (used on frame start)."""
        self._staged.clear()

    def clear_objects(self):
        for ob in self._objects:
            try:
                ob.visible = False
            except Exception:
                pass
        self._objects.clear()

    def _create_vpython_box(self, pos, col):
        return box(
            pos=vector(*pos), size=vector(4, 4, 4), color=col, visible=True
        )

    def _render_triangles_for_shape(self, shape_id, pos, rot):
        tris = self.shape_registry.get(shape_id, [])
        tri_objs = []
        for tri in tris:
            v0, v1, v2, colors = tri
            try:
                rv0 = [
                    rot[0] * v0[0] + rot[1] * v0[1] + rot[2] * v0[2],
                    rot[3] * v0[0] + rot[4] * v0[1] + rot[5] * v0[2],
                    rot[6] * v0[0] + rot[7] * v0[1] + rot[8] * v0[2],
                ]
                rv1 = [
                    rot[0] * v1[0] + rot[1] * v1[1] + rot[2] * v1[2],
                    rot[3] * v1[0] + rot[4] * v1[1] + rot[5] * v1[2],
                    rot[6] * v1[0] + rot[7] * v1[1] + rot[8] * v1[2],
                ]
                rv2 = [
                    rot[0] * v2[0] + rot[1] * v2[1] + rot[2] * v2[2],
                    rot[3] * v2[0] + rot[4] * v2[1] + rot[5] * v2[2],
                    rot[6] * v2[0] + rot[7] * v2[1] + rot[8] * v2[2],
                ]
                tri_obj = triangle(
                    v0=vertex(
                        pos=vector(*rv0) + vector(*pos),
                        color=vector(*colors[0]),
                    ),
                    v1=vertex(
                        pos=vector(*rv1) + vector(*pos),
                        color=vector(*colors[1]),
                    ),
                    v2=vertex(
                        pos=vector(*rv2) + vector(*pos),
                        color=vector(*colors[2]),
                    ),
                )
                tri_objs.append(tri_obj)
            except Exception:
                self.log("Error creating triangle")
        return tri_objs

    def render_frame(self):
        self.log("Render frame. staged", len(self._staged))
        if self.debug:
            print(
                f"[RENDERER] Using shapes: {list(self.shape_registry.keys())}, staged: {len(self._staged)}"
            )
        self.clear_objects()
        for desc in self._staged:
            shape_id = desc["id"]
            pos = desc["pos"]
            rot = desc["rot"]
            if self.use_custom_shapes and shape_id in self.shape_registry:
                tri_objs = self._render_triangles_for_shape(shape_id, pos, rot)
                self._objects.extend(tri_objs)
                if self.debug:
                    print(
                        f"[RENDERER] Rendered {len(tri_objs)} triangles for shape {shape_id}"
                    )
            else:
                col = color.green if shape_id == 0 else color.red
                self._objects.append(self._create_vpython_box(pos, col))
        self._staged.clear()

    def set_camera(self, pos, rot):
        """Set camera position and orientation"""
        try:
            scene.fov = math.radians(90)
            axis = vector(rot[2], -rot[5], rot[8]).norm()
            scene.camera.axis = axis
            scene.range = 100
            up = vector(rot[1], -rot[4], rot[7]).norm()
            scene.camera.up = up
            scene.camera.pos = vector(pos[0], -pos[1], -pos[2])
        except Exception:
            self.log("Failed to set camera")

    def stop(self):
        # Hide objects and better ensure VPython scene gets cleaned up
        try:
            scene.visible = False
        except Exception:
            pass
        self.clear_objects()
