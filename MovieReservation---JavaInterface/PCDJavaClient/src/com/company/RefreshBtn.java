package com.company;

import javax.swing.*;
import java.io.*;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.util.ArrayList;

public class RefreshBtn extends JButton implements Command {
    protected int portNumber;
    protected JTextField msgField;
    protected DefaultListModel<String> movieList;

    public RefreshBtn(int portNumber, JTextField msgField, DefaultListModel<String> l1){
        super("Refresh");
        this.portNumber = portNumber;
        this.msgField = msgField;
        this.movieList = l1;
    }

    public void parseString(String str){
        String[] strings = str.split("\n");
        this.movieList.clear();
        for(String s : strings){
            movieList.addElement(s);
        }
    }

    @Override
    public void execute() {
        Socket socket = null;
        byte[] buffer = new byte[1024];
        int read;

        try {
            socket = new Socket("localhost", this.portNumber);
            socket.getOutputStream().write("1".getBytes("US-ASCII"));
            read = socket.getInputStream().read(buffer);
            if(read !=-1){
                String output = new String(buffer, 0, read);
                parseString(output);
            }
            socket.close();
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
