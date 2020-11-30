package com.company;

import com.sun.corba.se.impl.protocol.JIDLLocalCRDImpl;

import javax.swing.*;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

public class ReservationBtn extends JButton implements Command {

    protected JList<String> movieList;
    protected JList<String> hourList;
    protected JTextField msgField;
    protected JTextField nameField;
    protected int portNumber;

    protected String movieChoice;
    protected String hourChoice;

    public ReservationBtn(JList<String> movieList, JList<String> hourList, JTextField nameField, JTextField msgField, int portNumber){
        super("Rezerva");
        this.movieList = movieList;
        this.hourList = hourList;
        this.msgField = msgField;
        this.nameField = nameField;
        this.portNumber = portNumber;
    }

    @Override
    public void execute() {
        this.movieChoice = DocumentManger.getInstance().getMovieChoice();
        this.hourChoice = DocumentManger.getInstance().getHourChoice().trim();
        if(nameField.getText().isEmpty()){
            msgField.setText("Nu ai introdus numele!");;
        }else if(movieChoice == null){
            msgField.setText("Nu ai selectat un film!");
        }else if(hourChoice == null){
            msgField.setText("Nu ai selectat o ora!");
        }else{
            Socket socket = null;
            byte[] buffer = new byte[1024];
            int read;
            String s1 = "";
            String str = "2";
            str += movieChoice.trim();
            str += hourChoice.substring(0,1);
            str += nameField.getText();
            try {
                socket = new Socket("localhost", this.portNumber);
                socket.getOutputStream().write(str.getBytes("US-ASCII"));
                read = socket.getInputStream().read(buffer);
                if(read !=-1){
                    String output = new String(buffer, 0, read);
                    msgField.setText(output);
                }
                socket.close();
            } catch (UnknownHostException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
