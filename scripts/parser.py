from HTMLParser import HTMLParser

# create a subclass and override the handler methods
class MyHTMLParser(HTMLParser):
    colors = []
    open_tag = ""
    color = ""
    name = ""
    def handle_starttag(self, tag, attrs):
        self.open_tag = tag
        if(tag=="font"):
            for attr in attrs:
                if(attr[0]=="color"):
                    color = attr[1].lstrip('#')
                    r = int(color[0:2], 16)
                    g = int(color[2:4], 16)
                    b = int(color[4:6], 16)
                    self.color = "{ %i , %i , %i };" % (r, g, b)

    def handle_endtag(self, tag):
        if(tag=="font"):
            self.colors.append((self.name,self.color))
        open_tag = ""
    def handle_data(self, data):
        if(self.open_tag=="font"):  
            data = data.replace(' ','')
            self.name = "RGB COLOR_%s = " % data



# RGB COLOR_RED = { 255 , 0 , 0 };

# instantiate the parser and fed it some HTML
parser = MyHTMLParser()
parser.feed(open('webcolors.html').read())

for el in parser.colors:
    print el[0]+el[1]

#print parser.colors
#print len(parser.colors)