import gdb
import graphviz
pp = gdb.printing.RegexpCollectionPrettyPrinter("suugu")

class MathObject_printer:
    def __init__(self, val):
        self.val = val;

    def to_string(self):
        name = f"{self.val['name']}"
        hash = f"{self.val['hash']}"
        desc = f"{self.val['description']}"
        type = f"{self.val['type']}"
        disp = '\t'.join(f"{self.val['display']}".splitlines(True))
        parts = f"{self.val['parts']}"

        return (
f"""
MathObject{{
    name  = {name};
    hash  = {hash};
    desc  = {desc};
    type  = {type};
    disp  = {disp};
    parts = {parts};
}}
"""
)
pp.add_printer("MathObject", "^MathObject$", MathObject_printer)

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)


class PrintTermTree(gdb.Command):
    def __init__(self):
        super(PrintTermTree, self).__init__("ptt", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)
        self.layers = 0

    def print_tree(self, node:gdb.Value):
        self.dot.node(str(node), str(node['raw']))
        current = node['first_child']
        while int(current):
            self.print_tree(current)
            self.dot.edge(str(node), str(current))
            current = current['next']

    def invoke(self, arg, tty):
        self.dot = graphviz.Digraph()
        val = gdb.parse_and_eval(arg)
        if val.type.name != "Term" and val.type.name != "Term*":
            print(f"ptt requires a Term or Term*, but got {val.type.name}")
        self.print_tree(val)
        self.dot.render('misc/termtree')

PrintTermTree()