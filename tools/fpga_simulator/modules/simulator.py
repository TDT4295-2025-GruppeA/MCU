# simulator.py
# Top-level FPGA visualizer / simulator.
#
# This module receives parsed SPI packets from `SerialReader` and dispatches
# them to the renderer. It keeps an internal state for in-progress uploads and
# exposes a top-level dispatcher `handle_command()` which the run wrapper calls
# when new packets arrive.

from .packets import *
from .renderer import Renderer


class Simulator:
    def __init__(self, renderer=None, debug=False):
        self.debug = debug
        self.renderer = renderer or Renderer(debug=debug)
        self.current_upload_id = None
        self.current_upload_tris = []

    def debug_log(self, *args):
        if self.debug:
            print(*args)

    def begin_upload(self, packet: bytes):
        # If an upload is already in progress, save it first (preserve previous shape)
        self.finish_upload()
        # model id in packet[1]
        self.current_upload_id = packet[1]
        self.current_upload_tris = []
        self.debug_log("begin upload", self.current_upload_id)

    def upload_triangle(self, packet: bytes):
        # parse triangles: packet structure is expected: 1 + color(2) + 12 bytes Q16*3
        # slice per triangle: 1 + color(2) + 12 bytes Q16*3
        colors = []
        verts = []
        for i in range(3):
            color_offset = 1 + i * 14
            color_val = int.from_bytes(
                packet[color_offset : color_offset + 2], "big"
            )
            r = ((color_val >> 10) & 0x1F) / 31.0
            g = ((color_val >> 5) & 0x1F) / 31.0
            b = (color_val & 0x1F) / 31.0
            colors.append((r, g, b))
            vertex_offset = color_offset + 2
            vert = parse_vertex(packet, vertex_offset)
            verts.append(vert)
        self.current_upload_tris.append((verts[0], verts[1], verts[2], colors))
        self.debug_log(
            "upload tri, now tri_count", len(self.current_upload_tris)
        )

    def finish_upload(self):
        if self.current_upload_id is not None and self.current_upload_tris:
            if self.debug:
                print(
                    f"[SIM_REF] Saving shape {self.current_upload_id} with {len(self.current_upload_tris)} tris"
                )
            self.renderer.save_shape(
                self.current_upload_id, self.current_upload_tris
            )
        self.current_upload_id = None
        self.current_upload_tris = []

    def add_instance(self, packet: bytes):
        is_last = packet[1] == 0x01
        shape_id = packet[2]
        pos = parse_vertex(packet, 3)
        rot = parse_rotation(packet, 15)
        self.renderer.stage_instance(shape_id, pos, rot, is_last)
        if is_last:
            self.renderer.render_frame()

    def position_camera(self, packet: bytes):
        try:
            pos = parse_vertex(packet, 3)
            rot = parse_rotation(packet, 15)
            # delegate to renderer
            self.renderer.set_camera(pos, rot)
            self.debug_log("camera pos", pos)
        except Exception:
            pass

    def frame_start(self):
        self.finish_upload()
        # Clear staging descriptors for a new frame (expected behavior for FRAME_START)
        try:
            self.renderer.clear_staging()
        except Exception:
            pass

    def reset(self):
        # clearing shapes
        self.renderer.shape_registry.clear()
        self.current_upload_id = None
        self.current_upload_tris = []

    # top-level dispatcher


def handle_command(sim: Simulator, packet: bytes):
    cmd = opcode_from_packet(packet)
    if cmd == CMD_BEGIN_UPLOAD:
        sim.begin_upload(packet)
    elif cmd == CMD_UPLOAD_TRIANGLE:
        sim.upload_triangle(packet)
    elif cmd == CMD_ADD_INSTANCE:
        sim.add_instance(packet)
    elif cmd == CMD_FRAME_START:
        sim.frame_start()
    elif cmd == CMD_FRAME_END:
        sim.renderer.render_frame()
    elif cmd == CMD_RESET:
        sim.reset()
    elif cmd == CMD_POSITION_CAMERA:
        sim.position_camera(packet)
    else:
        if sim.debug:
            print("Unknown opcode", cmd)
