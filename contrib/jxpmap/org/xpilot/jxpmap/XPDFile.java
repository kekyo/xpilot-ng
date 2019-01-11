package org.xpilot.jxpmap;

import java.util.Iterator;
import java.util.ArrayList;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.DataInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.io.File;

public class XPDFile extends ArrayList {

    public void save(File file) throws IOException {
        OutputStream out = new GZIPOutputStream(new FileOutputStream(file));
        try {
            out.write(("XPD " + size() + "\n").getBytes("ISO8859_1"));
            for (Iterator i = iterator(); i.hasNext();) {
                Part p = (Part)i.next();
                out.write((p.name + " " + p.data.length + "\n")
                    .getBytes("ISO8859_1"));
                out.write(p.data);
            }
        } finally {
            out.close();
        }
    }
    
    public static XPDFile load(File file) throws IOException, ParseException {
        XPDFile xpd = new XPDFile();
        DataInputStream in = 
            new DataInputStream(
                new GZIPInputStream(
                    new FileInputStream(file)));
        try {
            String header = readLine(in);
            if (!header.startsWith("XPD "))
                throw new ParseException("invalid file header");
            int count = Integer.parseInt(header.substring(4));
            for (int i = 0; i < count; i++) {
                String info = readLine(in);
                int mark = info.lastIndexOf(' ');
                if (mark == -1) throw new ParseException(
                    "invalid part info: " + info);
                String name = info.substring(0, mark);
                int size = Integer.parseInt(info.substring(mark + 1));
                byte[] data = new byte[size];
                in.readFully(data);
                xpd.add(new Part(name, data));
            }
            return xpd;
        } catch (NumberFormatException nfe) {
            nfe.printStackTrace();
            throw new ParseException("corrupted xpd file");
        } finally {
            in.close();
        }
    }
    
    private static String readLine(InputStream in) 
    throws IOException, ParseException {
        StringBuffer line = new StringBuffer();
        while(true) {
            int c = in.read();
            if (c == -1) throw new ParseException(
                "unexpected end of file");
            if (c == '\n') return line.toString();
            line.append((char)c);
        }
    }

    public static class ParseException extends Exception {
        public ParseException(String msg) {
            super(msg);
        }
    }

    public static class Part {
        public final String name;        
        public final byte[] data;
        public Part(String name, byte[] data) {
            this.name = name;
            this.data = data;
        }
    }
}
