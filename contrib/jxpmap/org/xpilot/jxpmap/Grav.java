/*
 * $Id: Grav.java,v 1.1 2003/09/01 07:08:11 juhal Exp $
 */
package org.xpilot.jxpmap;

import java.awt.GridLayout;
import java.io.IOException;
import java.io.PrintWriter;
import java.math.BigDecimal;

import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JTextField;

/**
 * @author jli
 */
public class Grav extends MapObject {
    
    private static final int TYPE_POS = 0;
    private static final int TYPE_NEG = 1;
    private static final int TYPE_CWISE = 2;
    private static final int TYPE_ACWISE = 3;
    private static final int TYPE_UP = 4;
    private static final int TYPE_DOWN = 5;
    private static final int TYPE_RIGHT = 6;
    private static final int TYPE_LEFT = 7;

    private static final String[] IMAGES = { 
        "grav_pos.gif", "grav_neg.gif", "grav_cwise.gif", "grav_acwise.gif", 
        "grav_up.gif", "grav_down.gif", "grav_right.gif", "grav_left.gif" 
    };
    
    private static final String[] TYPES = {
        "pos", "neg", "cwise", "acwise", "up", "down", "right", "left"        
    };

    private int type;
    private BigDecimal force;


    public Grav () {
        this(0, 0, "pos", "2.0"); 
    }


    public Grav (int x, int y, String typeStr, String forceStr) {
        super(null, x, y, 35 * 64, 35 * 64);
        force = new BigDecimal(forceStr);
        for (int i = 0; i < TYPES.length; i++) {
            if (typeStr.equals(TYPES[i])) {
                setType(i);
                break;
            }
        }
    }

    public int getType() {
        return type;
    }

    public void setType(int i) {
        if (i > TYPE_LEFT) 
            throw new IllegalArgumentException("illegal grav type: " + i);
        type = i;
        setImage(IMAGES[i]);
    }
    
    public BigDecimal getForce() {
        return force;
    }

    public void setForce(BigDecimal decimal) {
        force = decimal;
    }
        
    public void printXml (PrintWriter out) throws IOException {
        out.print("<Grav x=\"");
        out.print(getBounds().x + getBounds().width / 2);
        out.print("\" y=\"");
        out.print(getBounds().y + getBounds().height / 2);
        out.print("\" type=\"");
        out.print(TYPES[getType()]);
        out.print("\" force=\"");
        out.print(force);
        out.println("\"/>");
    }

    
    public EditorPanel getPropertyEditor (MapCanvas c) {
        return new GravPropertyEditor(c);
    }


    private class GravPropertyEditor extends EditorPanel {

        private JComboBox cmbType;
        private JTextField fldForce;
        private MapCanvas canvas;


        public GravPropertyEditor (MapCanvas canvas) {

            setTitle("Grav");

            cmbType = new JComboBox();
            for (int i = 0; i <= TYPE_LEFT; i++) 
                cmbType.addItem(TYPES[i]);
            cmbType.setSelectedIndex(getType());
            fldForce = new JTextField(10);
            fldForce.setText(force.toString());
            setLayout(new GridLayout(2, 2));
            add(new JLabel("Type"));
            add(cmbType);
            add(new JLabel("Force"));
            add(fldForce);
            
            this.canvas = canvas;
        }
        
        
        public boolean apply () {
            int newType = cmbType.getSelectedIndex() ;
            BigDecimal newForce = new BigDecimal(fldForce.getText());
            canvas.setGravProperties(Grav.this, newType, newForce);
            return true;
        }
    }        
}
