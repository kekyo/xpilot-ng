package org.xpilot.jxpmap;

import java.awt.GridLayout;
import java.io.IOException;
import java.io.PrintWriter;

import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JSpinner;

public class Base extends MapObject {

    private static final int LEFT = 0;
    private static final int DOWN = 1;
    private static final int RIGHT = 2;
    private static final int UP = 3;

    private static final String[] IMAGES = 
    { "base_left.gif", "base_down.gif", "base_right.gif", "base_up.gif" };

    private int dir;
    private int team;
    private int order;

    public Base () {
        this(0, 0, 32, 0, 0);
    }


    public Base (int x, int y, int dir, int team, int order) {
        super(null, x, y, 35 * 64, 35 * 64);
        setDir(dir);
        setTeam(team);
        setOrder(order);
    }


    public int getDir () {
        return dir;
    }


    public void setDir (int dir) {
        
        if (dir > 127) dir = 127;
        if (dir < 0) dir = 0;
        setImage(IMAGES[getOrientationByDir(dir)]);
        this.dir = dir;
    }


    public int getTeam () {
        return team;
    }


    public void setTeam (int team) {
        if (team < -1 || team > 10)
            throw new IllegalArgumentException
                ("illegal team: " + team);
        this.team = team;
    }
    
    public int getOrder() {
        return order;
    }
    
    public void setOrder(int order) {
        this.order = order;
    }

    
    public void printXml (PrintWriter out) throws IOException {
        out.print("<Base x=\"");
        out.print(getBounds().x + getBounds().width / 2);
        out.print("\" y=\"");
        out.print(getBounds().y + getBounds().height / 2);
        out.print("\" team=\"");
        out.print(getTeam());
        out.print("\" dir=\"");
        out.print(getDir());
        out.print("\" order=\"");
        out.print(getOrder());
        out.println("\"/>");
    }

    
    public EditorPanel getPropertyEditor (MapCanvas c) {
        return new BasePropertyEditor(c);
    }


    private int getOrientationByDir (int dir) {
        if (dir < 16) return LEFT;
        if (dir < 48) return DOWN;
        if (dir < 80) return RIGHT;
        if (dir < 112) return UP;
        return LEFT;
    }


    private class BasePropertyEditor extends EditorPanel {

        private JComboBox cmbTeam;
        private JComboBox cmbDir;
        private JSpinner spnOrder;
        private MapCanvas canvas;


        public BasePropertyEditor (MapCanvas canvas) {

            setTitle("Base");

            cmbTeam = new JComboBox();
            for (int i = -1; i <= 10; i++) 
                cmbTeam.addItem(new Integer(i));
            cmbTeam.setSelectedIndex(getTeam() + 1);
            
            cmbDir = new JComboBox();
            cmbDir.addItem("LEFT");
            cmbDir.addItem("DOWN");
            cmbDir.addItem("RIGHT");
            cmbDir.addItem("UP");
            cmbDir.setSelectedIndex(getOrientationByDir(getDir()));
            
            spnOrder = new JSpinner();
            spnOrder.setValue(new Integer(getOrder()));
            
            setLayout(new GridLayout(3,2));
            add(new JLabel("Team:"));
            add(cmbTeam);
            add(new JLabel("Direction:"));
            add(cmbDir);
            add(new JLabel("Order:"));
            add(spnOrder);
            this.canvas = canvas;
        }
        
        
        public boolean apply () {
            int newTeam = cmbTeam.getSelectedIndex() - 1;
            int newDir = cmbDir.getSelectedIndex() * 32;
            int newOrder = ((Integer)spnOrder.getValue()).intValue();
            if (newTeam != getTeam() 
                || newDir != getDir() 
                || newOrder != getOrder())
                canvas.setBaseProperties(
                    Base.this, newTeam, newDir, newOrder);
            return true;
        }
    }
}
