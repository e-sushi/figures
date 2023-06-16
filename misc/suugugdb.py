import gdb
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
