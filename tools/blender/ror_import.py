# <pep8-80 compliant>

bl_info = {
    "name": "RoR Importer",
    "author": "ulteq",
    "version": (0, 0, 1),
    "blender": (2, 79, 0),
    "category": "RoR",
}

import bpy
import json
import bmesh
from bpy.props import StringProperty
from bpy_extras.io_utils import ImportHelper

def register():
    bpy.app.debug = True
    bpy.utils.register_class(ror_import)
    bpy.types.INFO_MT_file_import.append(menu_func)
    return

def unregister():
    bpy.types.INFO_MT_file_import.remove(menu_func)
    bpy.utils.unregister_class(ror_import)
    return

def menu_func(self, context):
    self.layout.operator(ror_import.bl_idname, text="Truck (.truck)")

class ror_import(bpy.types.Operator, ImportHelper):
    bl_idname = "import_truck.truck"
    bl_label = "Import RoR Truck"
    filename_ext = ""
    filter_glob = StringProperty(
            default="*.truck;*.trailer;*.load;*.car;*.boat;*.airplane;*.train;*.machine;*.fixed",
            options={'HIDDEN'},
            )
    filepath = bpy.props.StringProperty(subtype="FILE_PATH")

    def execute(self, context):
        truckfile = []
        node_idx = 0
        nodes = []
        beam_idx = 0
        beams = []
        cab_idx = 0
        cabs = []

        with open(self.filepath, 'r') as f:
            node_defaults = ''
            beam_defaults = ''
            mode = 'ignore'
            groups = []
            for line in f:
                line = line.strip()
                if not line or line[0] == ';':
                    if mode == 'nodes' and line[:5] == ';grp:':
                        groups = [g.strip() for g in line[5:].split(',')]
                    elif mode == 'beams' and line[:5] == ';grp:':
                        pass
                    else:
                        truckfile.append(line)
                    continue

                args = line.replace(',', ' ').split()
                if not args or "set_" in args[0]:
                    if args and mode == 'nodes' and "set_n" in args[0]:
                        node_defaults = line
                    if args and mode == 'beams' and "set_b" in args[0]:
                        beam_defaults = line
                    else:
                        truckfile.append(line)
                    continue

                if args[0] == 'nodes':
                    mode = 'nodes'
                    node_defaults = ''
                    node_idx = len(truckfile)
                    continue
                elif args[0] == 'beams':
                    mode = 'beams'
                    beam_defaults = ''
                    beam_idx = len(truckfile)
                    continue
                elif args[0] == 'cab':
                    mode = 'cab'
                    cab_idx = len(truckfile)
                    continue
                elif not args[0].isdigit() or mode == 'ignore':
                    truckfile.append(line)
                    mode = 'ignore'

                if mode == 'nodes':
                    nodes.append([node_defaults] + [groups] + args[1:])
                elif mode == 'beams':
                    beams.append([beam_defaults] + args)
                elif mode == 'cab':
                    cabs.append(args)

        mesh = bpy.data.meshes.new(self.filepath + "-mesh")
        obj  = bpy.data.objects.new(self.filepath + "-obj", mesh)

        bpy.context.scene.objects.link(obj)
        bpy.context.scene.objects.active = obj
        obj.select = True

        bpy.types.Object.RoRTruckFile = bpy.props.StringProperty()
        bpy.context.active_object.RoRTruckFile = json.dumps(truckfile)

        if (beam_idx < node_idx):
            beam_idx = len(truckfile)
        if (cab_idx < beam_idx):
            cab_idx = len(truckfile)
        indices = [node_idx, beam_idx, cab_idx]
        bpy.types.Object.RoRInsertIndices = bpy.props.StringProperty()
        bpy.context.active_object.RoRInsertIndices = json.dumps(indices)

        mesh = bpy.context.object.data
        bm   = bmesh.new()
        dl   = bm.verts.layers.deform.verify()

        defaults_key = bm.verts.layers.string.new("defaults")
        options_key = bm.verts.layers.string.new("options")
        for n in nodes:
            try:
                v = bm.verts.new((float(n[4]), float(n[2]), float(n[3])))
                bm.verts.ensure_lookup_table()
                bm.verts.index_update()
                bm.verts[-1][defaults_key] = n[0].encode()
                bm.verts[-1][options_key] = ' '.join(n[5:]).encode()
                for g in n[1]:
                    vg = obj.vertex_groups.get(g)
                    if not vg:
                        vg = obj.vertex_groups.new(g)
                    v[dl][vg.index] = 1.0
            except:
                print ("Failed to add vertex:", n)

        defaults_key = bm.edges.layers.string.new("defaults")
        options_key = bm.edges.layers.string.new("options")
        for b in beams:
            try:
                bm.edges.new((bm.verts[int(i)] for i in b[1:3]))
                bm.edges.ensure_lookup_table()
                bm.edges[-1][defaults_key] = b[0].encode()
                bm.edges[-1][options_key] = ' '.join(b[3:]).encode()
            except:
                print ("Failed to add edge:", b)

        options_key = bm.faces.layers.string.new("options")
        for c in cabs:
            try:
                bm.faces.new((bm.verts[int(i)] for i in c[:3]))
                bm.faces.ensure_lookup_table()
                bm.faces[-1][options_key] = ' '.join(c[3:]).encode()
            except:
                print ("Failed to add face:", c)

        bm.to_mesh(mesh)
        bm.free()

        return {'FINISHED'}
