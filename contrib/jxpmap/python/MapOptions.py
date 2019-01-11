from javastuff import *
class MapOptions:
    def __init__(self, opts = None):
        self.opts = {'mapwidth':'3500', 'mapheight':'3500', 'edgewrap':'yes'}
        if opts:
            self.opts.update(opts)
        self.updated()

    def updated(self):
        self.size = Dimension()
        self.size.width = int(self.opts['mapwidth']) * 64
        self.size.height = int(self.opts['mapheight']) * 64
        self.edgeWrap = self.opts['edgewrap'] == 'yes'

    def printXml(self, file):
        file.write('<GeneralOptions>\n')
        # Python 2.2 would have xml.sax.saxutils.quoteattr
        for o in [(x[0], quoteattr(x[1])) for x in self.opts.items()]:
            file.write('<Option name="%s" value="%s"/>\n' % o)
        file.write('</GeneralOptions>\n')
