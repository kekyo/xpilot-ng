class Pixmap:
    def __init__(self, bmstore, filename):
        self.scalable = 1
        self.fileName = filename
        self.image = bmstore[filename]

    def printXml(self, file):
        file.write('<BmpStyle id="%s" filename="%s" flags="%d"/>\n' %
                   (self.fileName, self.fileName, self.scalable > 0))
