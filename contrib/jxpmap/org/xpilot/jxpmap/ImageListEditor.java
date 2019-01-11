package org.xpilot.jxpmap;

import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;

import javax.swing.DefaultListModel;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.SwingConstants;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

public class ImageListEditor extends EditorPanel
    implements ListSelectionListener, ActionListener {

    private JLabel imgLabel;
    private DefaultListModel jlistModel;
    private JList jlist;
    private JButton btnAdd;
    private JButton btnEdit;
    private JButton btnDel;
    private List pixmaps;
    

    public ImageListEditor (List pixmaps) {

        setTitle("Image Manager");

        this.pixmaps = pixmaps;

        jlistModel = new DefaultListModel();
        for (int i = 0; i < pixmaps.size(); i++) 
            jlistModel.addElement(((Pixmap)pixmaps.get(i)).getFileName());
        jlist = new JList(jlistModel);
        jlist.addListSelectionListener(this);
        jlist.setFixedCellWidth(100);
        JScrollPane sp = new JScrollPane(jlist);
        
        setLayout(new GridLayout(1,2));
        JPanel p1 = new JPanel(new BorderLayout(5, 5));
        JPanel p2 = new JPanel();
        JPanel p3 = new JPanel(new GridLayout(1,3));
        
        btnAdd = new JButton("Add");
        btnAdd.addActionListener(this);
        btnEdit = new JButton("Edit");
        btnEdit.addActionListener(this);
        btnDel = new JButton("Del");
        btnDel.addActionListener(this);

        p3.add(btnAdd);
        p3.add(btnEdit);
        p3.add(btnDel);

        p2.add(p3);
        p1.add(p2, BorderLayout.SOUTH);
        p1.add(sp, BorderLayout.CENTER);
        add(p1);

        imgLabel = new JLabel();
        imgLabel.setHorizontalAlignment(SwingConstants.CENTER);
        add(imgLabel);
    }


    private void showImage (Image img) {
        imgLabel.setIcon(new ImageIcon(img));
    }


    public void valueChanged (ListSelectionEvent evt) {
        int index = jlist.getSelectedIndex();
        if (index == -1) return;
        showImage(((Pixmap)pixmaps.get(index)).getImage());
    }


    public void actionPerformed (ActionEvent evt) {
        Object src = evt.getSource();

        if (src == btnAdd) {

            Pixmap pixmap = new Pixmap();
            EditorDialog.show(this, new PixmapEditor(pixmap), true);
            if (pixmap.getFileName() != null) {
                pixmaps.add(pixmap);
                jlistModel.addElement(pixmap.getFileName());
            }
            return;
            
            
        } else if (src == btnEdit) {

            int index = jlist.getSelectedIndex();
            if (index == -1) {
                JOptionPane.showMessageDialog
                    (this, "First select an image from the list", 
                     "Information", JOptionPane.INFORMATION_MESSAGE);
                return;
            }
            Pixmap pixmap = (Pixmap)pixmaps.get(index);
            EditorDialog.show(this, new PixmapEditor(pixmap), true);
            jlistModel.set(index, pixmap.getFileName());
            return;


        } else if (src == btnDel) {

            int index = jlist.getSelectedIndex();
            if (index == -1) {
                JOptionPane.showMessageDialog
                    (this, "First select an image from the list", 
                     "Information", JOptionPane.INFORMATION_MESSAGE);
                return;
            }
            pixmaps.remove(index);
            jlistModel.remove(index);
            return;
        }
    }
}
