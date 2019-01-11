package org.xpilot.jxpmap;

import java.awt.image.BufferedImage;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;

/**
 * PPMDecoder, based on Acme.JPM.Decoders.PpmDecoder of Jef Poskanzer
 * http://www.acme.com/resources/classes/Acme/JPM/Decoders/PpmDecoder.java
 */
public class PPMDecoder 
{
    private static final int PBM_ASCII = 1;
    private static final int PGM_ASCII = 2;
    private static final int PPM_ASCII = 3;
    private static final int PBM_RAW = 4;
    private static final int PGM_RAW = 5;
    private static final int PPM_RAW = 6;
    
    public BufferedImage decode (InputStream in) throws IOException 
    {
        int type = -1, maxval = -1;

        char c1 = (char) readByte(in);
        char c2 = (char) readByte(in); 
        if (c1 != 'P') throw new IOException("not a PBM/PGM/PPM file");

        switch (c2)
        {
        case '1':
            type = PBM_ASCII;
            break;
        case '2':
            type = PGM_ASCII;
            break;
        case '3':
            type = PPM_ASCII;
            break;
        case '4':
            type = PBM_RAW;
            break;
        case '5':
            type = PGM_RAW;
            break;
        case '6':
            type = PPM_RAW;
            break;
        default:
            throw new IOException("not a standard PBM/PGM/PPM file");
        }

        int width = readInt(in);
        int height = readInt(in);
        if (type != PBM_ASCII && type != PBM_RAW) maxval = readInt(in);


        BufferedImage img = 
            new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        
        for (int row = 0; row < height; row++) 
        {
            int col, r, g, b;
            int rgb = 0;
            char c; 

            for (col = 0; col < width; ++col)
            {
                switch (type)
                {
                case PBM_ASCII:
                    c = readChar(in);
                    if (c == '1')
                        rgb = 0xff000000;
                    else if (c == '0')
                        rgb = 0xffffffff;
                    else
                        throw new IOException("illegal PBM bit");
                    break;
                case PGM_ASCII:
                    g = readInt(in);
                    rgb = makeRgb(g, g, g);
                    break;
                case PPM_ASCII:
                    r = readInt(in);
                    g = readInt(in);
                    b = readInt(in);
                    rgb = makeRgb(r, g, b);
                    break;
                case PBM_RAW:
                    if (readBit(in))
                        rgb = 0xff000000;
                    else
                        rgb = 0xffffffff;
                    break;
                case PGM_RAW:
                    g = readByte(in);
                    if (maxval != 255)
                        g = fixDepth(g, maxval);
                    rgb = makeRgb(g, g, g);
                    break;
                case PPM_RAW:
                    r = readByte(in);
                    g = readByte(in);
                    b = readByte(in);
                    if (maxval != 255)
                    {
                        r = fixDepth(r, maxval);
                        g = fixDepth(g, maxval);
                        b = fixDepth(b, maxval);
                    }
                    rgb = makeRgb(r, g, b);
                    break;
                }
                img.setRGB(col, row, rgb);
            }
        }

        return img;
    }


    private int readByte (InputStream in) throws IOException
    {
        int b = in.read();
        if (b == -1)
            throw new EOFException();
        return b;
    }     


    private int bitshift = -1;
    private int bits;
    private boolean readBit (InputStream in) throws IOException
    {
        if (bitshift == -1)
        {
            bits = readByte(in);
            bitshift = 7;
        }
        boolean bit = (((bits >> bitshift) & 1) != 0);
        --bitshift;
        return bit;
    }    


    private char readChar (InputStream in) throws IOException
    {
        char c = (char) readByte(in);
        if (c == '#')
        {
            do
            {
                c = (char) readByte(in);
            }
            while (c != '\n' && c != '\r');
        } return c;
    }     


    private char readNonwhiteChar (InputStream in) throws IOException
    {
        char c; 
        do
        {
            c = readChar(in);
        }
        while (c == ' ' || c == '\t' || c == '\n' || c == '\r'); 
        return c;
    }


    private int readInt (InputStream in) throws IOException
    {
        char c;
        int i; 

        c = readNonwhiteChar(in);
        if (c < '0' || c > '9')
            throw new IOException("junk in file where integer should be");
        
        i = 0;
        do
        {
            i = i * 10 + c - '0';
            c = readChar(in);
        }
        while (c >= '0' && c <= '9'); return i;
    }


    private int fixDepth (int p, int maxval)
    {
        return (p * 255 + maxval / 2) / maxval;
    }


    private int makeRgb (int r, int g, int b)
    {
        return 0xff000000 | (r << 16) | (g << 8) | b;
    }
}
