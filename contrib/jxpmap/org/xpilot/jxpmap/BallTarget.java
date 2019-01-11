/*
 * $Id: BallTarget.java,v 1.3 2004/11/18 19:55:54 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Collection;

/**
 * @author jli
 */
public class BallTarget extends Group {
    
    public BallTarget() {
        super();
    }
    
    public BallTarget(Collection c) {
        super(c);
    }    
    
    public BallTarget(Collection c, int team) {
        super(c);
        setTeam(team);
    }

    public void printXml(PrintWriter out) throws IOException {
        out.println("<BallTarget team=\"" + getTeam() + "\">");
        super.printMemberXml(out);
        out.println("</BallTarget>");
    }

    public EditorPanel getPropertyEditor(MapCanvas canvas) {
        return new TeamEditor("Team", canvas, this);
    }
}
