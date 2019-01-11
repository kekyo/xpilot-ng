/*
 * $Id: Target.java,v 1.3 2004/11/18 19:55:54 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;

public class Target extends Group {
    
    public Target() {
        super();
    }
    
    public Target(Collection c) {
        super(c);
    }
    
    public Target(Collection c, int team) {
        super(c);
        setTeam(team);
    }

    public void printXml(PrintWriter out) throws IOException {
        out.println("<Target team=\"" + getTeam() + "\">");
        super.printMemberXml(out);
        out.println("</Target>");
    }

    public EditorPanel getPropertyEditor(MapCanvas canvas) {
        return new TeamEditor("Target", canvas, this);
    }
}
