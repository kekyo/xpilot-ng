/*
 * $Id: Decoration.java,v 1.2 2004/11/18 19:55:54 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;

/**
 * @author jli
 */
public class Decoration extends Group  {
    
    public Decoration() {
        super();
    }
    
    public Decoration(Collection c) {
        super(c);
    }

    public void printXml(PrintWriter out) throws IOException {
        out.println("<Decor>");
        super.printMemberXml(out);
        out.println("</Decor>");
    }
}
