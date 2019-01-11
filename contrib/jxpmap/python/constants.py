attrs = "name has_team".split()
num = 0
class Polytype:
    def __init__(self, *a):
        global num
        self.num = num
        num += 1
        for name, value in zip(attrs, a):
            setattr(self, name, value)

polytypes = [Polytype(*p) for p in (
    ("Normal", 0),
    ("Ball area", 0),
    ("Ball target", 1),
    ("Decoration", 0),
    )]
