package org.xpilot.jxpmap;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;

public class MapObjectPopup extends JPopupMenu implements ActionListener {


    protected MapCanvas canvas;
    protected MapObject object;
    private JMenuItem itemToFront;
    private JMenuItem itemToBack;
    private JMenuItem itemRemove;
    private JMenuItem itemProperties;


    public MapObjectPopup (MapObject obj) {
        super();
        object = obj;

        JMenuItem item;
        item = new JMenuItem("Bring to front");
        item.addActionListener(this);
        add(item);
        itemToFront = item;

        item = new JMenuItem("Send to back");
        item.addActionListener(this);
        add(item);
        itemToBack = item;
        
        addSeparator();
         
        item = new JMenuItem("Remove");
        item.addActionListener(this);
        add(item);
        itemRemove = item;

        addSeparator();

        item = new JMenuItem("Properties");
        item.addActionListener(this);
        add(item);
        itemProperties = item;
    }


    public void show (MapCanvas canvas, int x, int y) {
        this.canvas = canvas;
        super.show(canvas, x, y);
    }


    public void actionPerformed (ActionEvent evt) {

        Object src = evt.getSource();
        if (src == itemToFront) {
            toFront();
        } else if (src == itemToBack) {
            toBack();
        } else if (src == itemRemove) {
            remove();
        } else if (src == itemProperties) {
            properties();
        }
    }
    
   
    protected void toFront () {
        canvas.bringMapObject(object, true);
        canvas.repaint();
    }


    protected void toBack () {
        canvas.bringMapObject(object, false);
        canvas.repaint();
    }


    protected void remove () {
        canvas.removeMapObject(object);
        canvas.repaint();
    }


    protected void properties () {
        EditorDialog.show(canvas, object);
        canvas.repaint();
    }
}
