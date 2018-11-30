# <pep8-80 compliant>

bl_info = {
    "name": "RoR Exporter",
    "author": "ulteq",
    "version": (0, 0, 1),
    "blender": (2, 79, 0),
    "category": "RoR",
}

import bpy
import json
import bmesh
from bpy_extras.io_utils import ExportHelper

def register():
    bpy.app.debug = True
    bpy.utils.register_class(ror_export)
    bpy.types.INFO_MT_file_export.append(menu_func)
    return

def unregister():
    bpy.types.INFO_MT_file_export.remove(menu_func)
    bpy.utils.unregister_class(ror_export)
    return

def menu_func(self, context):
    self.layout.operator(ror_export.bl_idname, text="Truck (.truck)")

class ror_export(bpy.types.Operator, ExportHelper):
    bl_idname = "export_truck.truck"
    bl_label = "Export RoR Truck"
    filename_ext = ".truck"
    filepath = bpy.props.StringProperty(subtype="FILE_PATH")

    def execute(self, context):
        nodes = []
        beams = []
        cabs = []

        for obj in context.selected_objects[:1]:
            if obj.type != 'MESH':
                continue

            bpy.ops.object.mode_set(mode="EDIT")
            bm = bmesh.from_edit_mesh(obj.data)

            group_names = {group.index : group.name for group in obj.vertex_groups}
            node_digits = len(str(len(obj.data.vertices) - 1))

            format_string = '{:'+str(node_digits)+'d}, {: 8.3f}, {: 8.3f}, {: 8.3f}'
            defaults_key = bm.verts.layers.string.get("defaults")
            options_key = bm.verts.layers.string.get("options")
            bm.verts.ensure_lookup_table()
            for v, bv in zip(obj.data.vertices, bm.verts):
                defaults = ''
                if defaults_key:
                    defaults = bv[defaults_key].decode()
                options = ''
                if options_key:
                    options = bv[options_key].decode()
                if not options:
                    options = 'n'
                groups = [group_names[g.group] for g in v.groups]
                nodes.append([format_string.format(v.index, v.co[1], v.co[2], v.co[0]), options, groups, defaults])

            format_string = '{:'+str(node_digits)+'d}, {:'+str(node_digits)+'d}'
            defaults_key = bm.edges.layers.string.get("defaults")
            options_key = bm.edges.layers.string.get("options")
            bm.edges.ensure_lookup_table()
            for e, be in zip(obj.data.edges, bm.edges):
                defaults = ''
                if defaults_key:
                    defaults = be[defaults_key].decode()
                options = ''
                if options_key:
                    options = be[options_key].decode()
                if not options:
                    options = 'v'
                ids = sorted([[g.group for g in obj.data.vertices[e.vertices[i]].groups] for i in [0, 1]])
                vg1, vg2 = [[group_names[g] for g in ids[i]] for i in [0, 1]]
                groups = vg1 if vg1 == vg2 else [', '.join(vg1)] + [">"] + [', '.join(vg2)]
                beams.append([ids, groups, format_string.format(e.vertices[0], e.vertices[1]), options, defaults])

            format_string = '{:'+str(node_digits)+'d}, {:'+str(node_digits)+'d}, {:'+str(node_digits)+'d}'
            options_key = bm.faces.layers.string.get("options")
            bm.faces.ensure_lookup_table()
            for p, bp in zip(obj.data.polygons, bm.faces):
                if len(p.vertices) == 3:
                    options = ''
                    if options_key:
                        options = bp[options_key].decode()
                    if not options:
                        options = 'c'
                    cabs.append([format_string.format(p.vertices[0], p.vertices[1], p.vertices[2]), options])

            bpy.ops.object.mode_set(mode="OBJECT")
            bm.free()

        truckfile = []
        indices = [0, 0, 0]
        try:
            truckfile = json.loads(bpy.context.active_object.RoRTruckFile)
            indices = json.loads(bpy.context.active_object.RoRInsertIndices)
        except:
            pass

        with open(self.filepath, 'w') as f:
            for line in truckfile[:indices[0]]:
                print (line, file=f)

            print("nodes", file=f)
            defaults = ''
            vertex_groups = []
            for n in sorted(nodes):
                if n[-1] != defaults:
                    defaults = n[-1]
                    print (defaults, file=f)
                if n[-2] != vertex_groups:
                    vertex_groups = n[-2]
                    print (";grp:", ', '.join(vertex_groups), file=f)
                print (*n[:-2], sep=', ', file=f)

            lines = truckfile[indices[0]:indices[1]]
            if not lines:
                lines = ['']
            for line in lines:
                print (line, file=f)

            print("beams", file=f)
            edge_groups = []
            for b in sorted(beams):
                if b[-1] != defaults:
                    defaults = b[-1]
                    print (defaults, file=f)
                if b[1] != edge_groups:
                    edge_groups = b[1]
                    print (";grp:", *edge_groups, file=f)
                print (*b[2:-1], sep=', ', file=f)

            lines = truckfile[indices[1]:indices[2]]
            if not lines:
                lines = ['']
            for line in lines:
                print (line, file=f)

            if cabs:
                print ("cab", file=f)
                for c in cabs:
                    print (*c, sep=', ', file=f)

            for line in truckfile[indices[2]:]:
                print (line, file=f)

        return {'FINISHED'}
