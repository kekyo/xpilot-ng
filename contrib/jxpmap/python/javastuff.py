import gtk
import GDK
import gdkpixbuf
import copy

# This contains classes/functions giving functionality of some Java
# libraries used in the original Java version, and some additional
# utility stuff.

class Rectangle:
    def __init__(self, x, y, w, h):
        self.x = x
        self.y = y
        self.width = w
        self.height = h

    def getBounds(self):
        return self

    def paint(self, di):
        c = copy.copy(self).transform(di.tx)
        gtk.draw_rectangle(di.area, di.gc, gtk.FALSE, c.x, c.y,
                           c.width, c.height)

    def transform(self, tx):
        p1 = tx(Point(self.x, self.y))
        p2 = tx(Point(self.x + self.width, self.y + self.height))
        self.x, self.y = p1.x, p1.y
        self.width = p2.x - p1.x
        self.height = p2.y - p1.y
        if self.width < 0:
            self.x += self.width
            self.width *= -1
        if self.height < 0:
            self.y += self.height
            self.height *= -1
        return self

    def contains(self, p):
        return self.x <= p.x <= self.x + self.width and\
               self.y <= p.y <= self.y + self.height

class Line:
    def __init__(self, p1, p2):
        self.p1 = p1
        self.p2 = p2

    def paint(self, di):
        p1 = di.tx(self.p1)
        p2 = di.tx(self.p2)
        gtk.draw_line(di.area, di.gc, p1.x, p1.y, p2.x, p2.y)

    def getBounds(self):
        p1 = self.p1
        p2 = self.p2
        mx = max(p1.x, p2.x)
        my = max(p1.y, p2.y)
        nx = min(p1.x, p2.x)
        ny = min(p1.y, p2.y)
        return Rectangle(nx, ny, mx - nx, my - ny)

class Point:
    def __init__(self, x = 0, y = 0):
        self.x = int(x)
        self.y = int(y)
        self.width = 0
        self.height = 0

class Polygon:
    def __init__(self):
        self.points = []
        self.npoints = 0

    def recalculate_bounds(self):
        if len(self.points) == 0:
            self.bounds = Rectangle(0, 0, 0, 0)
            return
        x = [p.x for p in self.points]
        y = [p.y for p in self.points]
        self.bounds = Rectangle(min(x), min(y), max(x)-min(x), max(y)-min(y))

    def addPoint(self, x, y):
        self.points.append(Point(x, y))
        self.npoints += 1
        self.recalculate_bounds()

    def getBounds(self):
        return self.bounds

    def transform(self, tx):
        self.points = [tx(p) for p in self.points]
        self.recalculate_bounds()

    def __copy__(self):
        c = Polygon()
        c.points = self.points[:] # Should be just numbers, no deepcopy
        c.npoints = self.npoints
        c.bounds = copy.copy(self.bounds)
        return c

    def contains(self, p):
        l = [Point(p2.x - p.x, p2.y - p.y) for p2 in self.points]
        p1 = l[-1]
        inside = 0
        for p2 in l:
            if p1.y < 0:
                if p2.y >= 0:
                    if p1.x > 0 and p2.x >= 0:
                        inside += 1
                    elif p1.x >= 0 or p2.x >= 0:
                        s = p1.y * (p1.x - p2.x) - p1.x * (p1.y - p2.y)
                        if s == 0:
                            return 1
                        if s > 0:
                            inside += 1
            elif p2.y <= 0:
                if p2.y == 0:
                    if p2.x == 0 or p1.y == 0 and (p1.x <= 0 and p2.x >= 0
                                     or p1.x >= 0 and p2.x <= 0):
                        return 1
                elif p1.x > 0 and p2.x >= 0:
                    inside += 1
                elif p1.x >= 0 or p2.x >= 0:
                    s = p1.y * (p1.x - p2.x) - p1.x * (p1.y - p2.y)
                    if s == 0:
                        return 1
                    if s < 0:
                        inside += 1
            p1 = p2
        return inside & 1


    def paint(self, di):
        gc = di.gc
        tx = di.tx

        trans = [tx(p) for p in self.points]
        points = [(t.x, t.y) for t in trans]
        gtk.draw_polygon(di.area, gc, gtk.FALSE, points)

class AffineTransform:
    def __init__(self, x = 0, y = 0):
        self.x = x
        self.y = y
        self.xscale = 1.
        self.yscale = 1.

    def __call__(self, p):
        return Point(p.x * self.xscale + self.x, p.y * self.yscale + self.y)

    def translate(self, nx, ny):
        self.x += nx * self.xscale
        self.y += ny * self.yscale

    def scale(self, nx, ny):
        self.xscale *= nx
        self.yscale *= ny

    def createInverse(self):
        result = AffineTransform()
        result.xscale = 1. / self.xscale
        result.yscale = 1. / self.yscale
        result.x = -self.x / self.xscale
        result.y = -self.y / self.yscale
        return result

class Dimension:
    pass

#Convert from a single int to a tuple (usable by GTK)
def gcolor(color):
    return (color>>8&65280,color&65280,color<<8&65280)

class MouseEventHandler:
    def mousePressed(self, evt):
        pass

    def mouseReleased(self, evt):
        pass

    def mouseMoved(self, evt):
        pass

def ptSeqDistSq(x1, y1, x2, y2, px, py):
    x2 = float(x2 - x1)
    y2 = float(y2 - y1)
    px = float(px - x1)
    py = float(py - y1)
    s = px * x2 + py * y2
    if s < 0:
        return px * px + py * py
    len = x2 * x2 + y2 * y2
    if s > len:
        return (px - x2)**2 + (py - y2)**2
    s = float(s) / len
    return (px - x2 * s)**2 + (py - y2 * s)**2

class ScaledBitmap:
    def __init__(self, img):
        self.img = img
        self.unscaled, mask = self.img.render_pixmap_and_mask()
        self.scaled = None
        self.scale = 1
    def __deepcopy__(self, memo):
        return self
    def __getitem__(self, scale = 1):
        if scale == 1:
            return self.unscaled
        if scale != self.scale:
            width = int(self.img.width * scale + .5)
            width += width == 0
            height = int(self.img.height * scale + .5)
            height += height == 0
            scaled = self.img.scale_simple(width, height,
                                                gdkpixbuf.INTERP_BILINEAR)
            self.scaled, mask = scaled.render_pixmap_and_mask()
            self.scale = scale
        return self.scaled

import os
import weakref
from config import PIXMAP_PATH
class ScaledBitmapStore:
    def __init__(self):
        self.store = weakref.WeakValueDictionary()
        self.scale = 1
    def __getitem__(self, filename):
        bm = self.store.get(filename)
        if bm is None:
            for path in PIXMAP_PATH:
                if os.path.exists(path + filename):
                    img = gdkpixbuf.new_from_file(path + filename)
                    break
            else:
                raise "Image file not found"
            bm = ScaledBitmap(img)
            self.store[filename] = bm
        return bm

def toRGB(color):
    s = hex(color).upper()[2:] # strip 0x
    s = "0" * (6 - len(s)) + s
    return s

# There would be xml.sax.saxutils.quoteaddr in python 2.2
def quoteattr(text):
    repl = (('&', '&amp;'), ('"', '&quot;'), ("'", '&apos;'),
	    ('<', '&lt;'), ('>', '&gt;'))
    for t, r in repl:
	text = text.replace(t, r)
    return text
