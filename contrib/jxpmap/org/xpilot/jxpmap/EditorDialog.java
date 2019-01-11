package org.xpilot.jxpmap;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JOptionPane;
import javax.swing.JPanel;

public class EditorDialog extends JDialog implements ActionListener {

    public static final int OK_CANCEL = 0;
    public static final int CLOSE = 1;

    private JButton btnOk;
    private JButton btnCancel;
    private EditorPanel editor;


    protected EditorDialog (Component parent, 
                            EditorPanel editor, 
                            boolean modal,
                            int buttons) {

        super(JOptionPane.getFrameForComponent(parent), modal);
        this.editor = editor;
        
        setTitle(editor.getTitle());
        
        JPanel p1 = null;
        if (buttons == CLOSE) {
            p1 = new JPanel(new GridLayout(1,1));
            btnOk = new JButton("Close");
            btnOk.addActionListener(this);
            p1.add(btnOk);
        } else {
            p1 = new JPanel(new GridLayout(1,2));

            btnOk = new JButton("OK");
            btnOk.addActionListener(this);
            p1.add(btnOk);
            
            btnCancel = new JButton("Cancel");
            btnCancel.addActionListener(this);
            p1.add(btnCancel);
        }

        JPanel p2 = new JPanel();
        p2.add(p1);

        getContentPane().add(p2, BorderLayout.SOUTH);

        editor.setBorder
            (BorderFactory.createCompoundBorder
             (BorderFactory.createRaisedBevelBorder(),
              BorderFactory.createEmptyBorder(5, 5, 5, 5)));
        
        getContentPane().add(editor, BorderLayout.CENTER);
    }


    public static void show (MapCanvas canvas, MapObject object) {
        show(canvas, object.getPropertyEditor(canvas), true, OK_CANCEL);
    }
    
    
    public static void show (Component parent, 
                             EditorPanel editor, 
                             boolean modal) {
        show(parent, editor, modal, OK_CANCEL);
    }
    
    
    public static void show (Component parent, 
                             EditorPanel editor, 
                             boolean modal,
                             int buttons) {

        EditorDialog dialog = new EditorDialog(parent, editor, modal, buttons);
        dialog.pack();
        dialog.setLocationRelativeTo(JOptionPane.getFrameForComponent(parent));
        dialog.setVisible(true);        
    }


    public void actionPerformed (ActionEvent evt) {

        if (evt.getSource() == btnOk) {
            if (!editor.apply()) return;
            
        } else if (evt.getSource() == btnCancel) {
            if (!editor.cancel()) return;
            
        } else {
            return;
        }
        
        dispose();
    }
}
