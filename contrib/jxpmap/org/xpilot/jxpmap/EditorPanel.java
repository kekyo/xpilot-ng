package org.xpilot.jxpmap;

import javax.swing.JPanel;

public class EditorPanel extends JPanel {

    protected String title;
    

    public String getTitle () {
        return title;
    }


    public void setTitle (String title) {
        this.title = title;
    }


    public boolean apply () {
        return true;
    }
    
    
    public boolean cancel () {
        return true;
    }
}
