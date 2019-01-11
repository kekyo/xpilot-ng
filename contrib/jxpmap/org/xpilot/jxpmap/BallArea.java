/*
 * $Id: BallArea.java,v 1.3 2004/11/18 19:55:54 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;

/**
 * @author jli
 */
public class BallArea extends Group {
    
    public BallArea() {
        super();
    }   
    
    public BallArea(Collection c) {
        super(c);
    }

    public void printXml(PrintWriter out) throws IOException {
        out.println("<BallArea>");
        super.printMemberXml(out);
        out.println("</BallArea>");
    }
}
