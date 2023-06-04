import gdb
pp = gdb.printing.RegexpCollectionPrettyPrinter("suugu")

gdb.printing.register_pretty_printer(gdb.current_objfile(), pp)