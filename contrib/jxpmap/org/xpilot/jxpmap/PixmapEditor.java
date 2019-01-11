package org.xpilot.jxpmap;

import java.awt.BorderLayout;

import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;

import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JFileChooser;
import javax.swing.JTextField;
import javax.swing.JButton;

import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;

public class PixmapEditor extends EditorPanel {
    
    private static File lastSelection = 
        new File(System.getProperty("user.dir"));

    private Pixmap pixmap;
    private JTextField nameField;
    private File imageFile;

    public PixmapEditor (Pixmap pixmap) {
        setTitle("Image");
        this.pixmap = pixmap;
        setLayout(new BorderLayout(5, 5));
        nameField = new JTextField(10);
        if (pixmap.getFileName() != null)
            nameField.setText(pixmap.getFileName());
        JButton browseButton = new JButton("Browse");
        browseButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                JFileChooser fc = new JFileChooser(lastSelection);
                if (fc.showOpenDialog(null) 
                    != JFileChooser.APPROVE_OPTION) 
                        return;
                imageFile = fc.getSelectedFile();
                if (imageFile == null) return;
                lastSelection = imageFile;
                nameField.setText(imageFile.getName());
            }
        });
        add(new JLabel("Name"), BorderLayout.WEST);
        add(nameField, BorderLayout.CENTER);
        add(browseButton, BorderLayout.EAST);
    }

    
    public boolean apply () {
        
        pixmap.setFileName(nameField.getText());
        
        /* I don't think non-scalable images are useful */ 
        pixmap.setScalable(true);
        
        if (imageFile == null && pixmap.getImage() == null) {
            JOptionPane.showMessageDialog
                (this, "Please, select an image using the Browse button.",
                 "Information", JOptionPane.INFORMATION_MESSAGE);
            return false;
        }
        if (imageFile != null) {
            try {
                InputStream in = new BufferedInputStream(
                    new FileInputStream(imageFile));
                try {
                    pixmap.load(in);
                    return true;
                } finally {
                    in.close();
                }
            } catch (Exception e) {
                JOptionPane.showMessageDialog
                    (this, "Failed to load " + imageFile.getName() + ".",
                     "Error", JOptionPane.ERROR_MESSAGE);
                return false;
            }
        }
        return true;
    }
}
