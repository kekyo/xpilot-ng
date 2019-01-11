from __future__ import nested_scopes
import gtk
import EditorDialog
from constants import polytypes

# The GtkOptionMenu has a rather clumsy interface.
# There might be a nicer widget giving the same functionality in the libraries

class PolygonPropertyEditor(gtk.GtkVBox):
    buttons = EditorDialog.OK_CANCEL
    def __init__(self, canvas, polygon):
        self.polygon = polygon
        self.canvas = canvas
        gtk.GtkVBox.__init__(self)
        self.values = {}
        def makeoptmenu(name, opts, default = 0, func = None):
            hbox = gtk.GtkHBox()
            label = gtk.GtkLabel(name)
            label.show()
            hbox.pack_start(label)
            menu = gtk.GtkMenu()
            def store(widget, data):
                self.values[name] = data
                if func:
                    func(widget, data)
            for text, data in opts:
                menu_item = gtk.GtkMenuItem(text)
                menu_item.connect("activate", store, data)
                menu_item.show()
                menu.append(menu_item)
            option_menu = gtk.GtkOptionMenu()
            option_menu.set_menu(menu)
            option_menu.set_history(default)
            self.values[name] = opts[default][1]
            option_menu.show()
            hbox.pack_start(option_menu)
            hbox.show()
            return hbox
        ps = canvas.model.polyStyles
        _ = [(style.id, style) for style in ps]
        self.pack_start(makeoptmenu("style", _, ps.index(polygon.style)))
        def check_team(widget = None, data = None):
            teammenu.set_sensitive(self.values["type"].has_team)
        _ = [(x.name, x) for x in polytypes]
        self.pack_start(makeoptmenu("type", _, polygon.type, check_team))
        _ = [(str(i), i) for i in range(1, 10)]
        teammenu = makeoptmenu("team", _, polygon.team - 1)
        self.pack_start(teammenu)
        check_team()
        self.show()

    def apply(self):
        self.polygon.style = self.values["style"]
        self.polygon.type = self.values["type"].num
        self.polygon.team = self.values["team"]
        self.canvas.repaint()
        return 1

    def cancel(self):
        return 1
