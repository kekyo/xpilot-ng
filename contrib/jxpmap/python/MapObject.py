import gdkpixbuf
import gtk
# The gdkpixbuf module doesn't seem too reliable, for example the
# render_pixmap_and_mask method seemed to always cause a segmentation fault
# if the gtk module had not been imported...
from javastuff import *
from MapObjectPopup import MapObjectPopup
import EditorDialog
import copy

class MapObject:
    def __init__(self, x, y, width, height, img):
        self.bounds = Rectangle(x - width / 2, y - height / 2, width, height)
        self.x = x
        self.y = y
        self.img = img

    def getBounds(self):
        return self.bounds

    def setBounds(self, bounds):
        self.bounds = bounds

    def contains(self, canvas, p):
        return len(canvas.wrapOffsets(self.getBounds(), p))

    def getZOrder(self):
        return 10

    def getCreateHandler(self):
        return CreateHandler(self)

    def moveTo(self, x, y):
        self.x = x
        self.y = y
        self.bounds.x = x - self.bounds.width / 2
        self.bounds.y = y - self.bounds.height / 2

    def getPreviewShape(self):
        return self.getBounds()

    def paint(self, di):
        if self.img is None:
            return
        r = self.getBounds()
        tx = di.tx
        pixmap = self.img[di.scale * 64]
        p = Point(r.x, r.y)
        p = tx(p)
        gtk.draw_pixmap(di.area, di.gc, pixmap, 0, 0, p.x, p.y - pixmap.height,
                            pixmap.width, pixmap.height)

    def checkEvent(self, canvas, me):
        if self.contains(canvas, me.point):
            if me.button == 1:
                if canvas.isErase():
                    canvas.getModel().removeObject(self)
                    canvas.repaint()
                    canvas.saveUndo()
                else:
                    canvas.setCanvasEventHandler(MoveHandler(self, me))
                return 1
            elif me.button == 3:
                MapObjectPopup(self, me.orig, canvas)
                return 1
        return 0

    def getPropertyEditor(self, canvas):
        class C(gtk.GtkLabel):
            def apply(self):
                return 1
        editor = C("No editable properties")
        editor.buttons = EditorDialog.CLOSE
        editor.show()
        return editor


class CreateHandler(MouseEventHandler):
    def __init__(self, object):
        self.object = object
        self.preview = object.getPreviewShape()
        self.toBeRemoved = None

    def mouseMoved(self, evt):
        c = evt.canvas
        gc = c.previewgc
        if self.toBeRemoved is not None:
            c.drawShape(self.toBeRemoved, gc)
        s = copy.copy(self.preview)
        s.transform(AffineTransform(evt.x - self.object.x,
                                    evt.y - self.object.y))
        c.drawShape(s)
        self.toBeRemoved = s

    def mousePressed(self, evt):
        c = evt.canvas
        c.getModel().addToFront(self.object)
        mapsize = c.model.options.size
        self.object.moveTo(evt.x % mapsize.width, evt.y % mapsize.height)
        c.setCanvasEventHandler(None)
        c.repaint()
        c.saveUndo()

class MoveHandler(MouseEventHandler):
    def __init__(self, obj, evt):
        self.object = obj
        self.offset = Point(evt.x - obj.x, evt.y - obj.y)
        self.preview = obj.getPreviewShape()
        self.toBeRemoved = None

    def mouseMoved(self, evt):
        c = evt.canvas
        if self.toBeRemoved is not None:
            c.drawShape(self.toBeRemoved)
        s = copy.copy(self.preview)
        s.transform(AffineTransform(evt.x - self.offset.x - self.object.x,
                                    evt.y - self.offset.y - self.object.y))
        c.drawShape(s)
        self.toBeRemoved = s

    def mouseReleased(self, evt):
        c = evt.canvas
        mapsize = c.model.options.size
        self.object.moveTo((evt.x - self.offset.x) % mapsize.width,
                           (evt.y - self.offset.y) % mapsize.height)
        c.setCanvasEventHandler(None)
        c.repaint()
        c.saveUndo()
