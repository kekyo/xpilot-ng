/*
 * $Id: TeamEditor.java,v 1.1 2003/08/24 18:37:36 juhal Exp $
 */
package org.xpilot.jxpmap;

import javax.swing.JComboBox;
import javax.swing.JLabel;

/**
 * @author jli
 */
public class TeamEditor extends EditorPanel {

    private JComboBox cmbTeam;
    private MapCanvas canvas;
    private MapObject object;

    public TeamEditor(String title, MapCanvas canvas, MapObject object) {
        setTitle(title);
        cmbTeam = new JComboBox();
        for (int i = -1; i <= 10; i++)
            cmbTeam.addItem(new Integer(i));
        cmbTeam.setSelectedIndex(object.getTeam() + 1);
        add(new JLabel("Team:"));
        add(cmbTeam);
        this.canvas = canvas;
        this.object = object;
    }

    public boolean apply() {
        int newTeam = cmbTeam.getSelectedIndex() - 1;
        if (newTeam != object.getTeam())
            canvas.setObjectTeam(object, newTeam);
        return true;
    }
}
