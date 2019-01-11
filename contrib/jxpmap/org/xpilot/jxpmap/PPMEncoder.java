package org.xpilot.jxpmap;

import java.awt.image.BufferedImage;

public class PPMEncoder {
    public byte[] encode(BufferedImage img) {
        int w = img.getWidth();
        int h = img.getHeight();
        byte[] header =("P6\n" + w + " " + h + "\n255\n").getBytes();
        int off = header.length;        
        byte[] data = new byte[off + h * w * 3];
        System.arraycopy(header, 0, data, 0, header.length);
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int i = off + (y * w + x) * 3;
                int rgb = img.getRGB(x, y);
                data[i    ] = (byte) ((rgb & 0xff0000) >> 16);
                data[i + 1] = (byte) ((rgb & 0x00ff00) >> 8);
                data[i + 2] = (byte) ( rgb & 0x0000ff);
            }
        }
        return data;
    }
}
