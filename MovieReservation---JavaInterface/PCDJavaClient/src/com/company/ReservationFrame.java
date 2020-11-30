package com.company;

import javax.swing.*;
import javax.swing.border.EmptyBorder;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

public class ReservationFrame extends JFrame implements ActionListener {

    protected int portNumber = 4444;

    protected JTextField name = new JTextField();
    protected JTextField msgField = new JTextField();

    protected JLabel nameLabel = new JLabel();
    protected JLabel movieLabel = new JLabel();
    protected JLabel hoursLabel = new JLabel();

    protected JList<String> movieList;
    protected JList<String> hourList;

    protected JPanel p1;//panel for browse button and text field
    protected JPanel panelMsg;//panel for text original/edited photo
    protected JPanel p2; //panel with text field and list views
    protected JPanel p3;//panel for message field
    protected JPanel p4;//panel fro buttons
    protected JPanel pAll; //panel where all the panels are addade on y axis

    protected ReservationBtn reservationBtn;
    protected RefreshBtn refreshBtn;

    protected String movieChoice;
    protected String hourChoice;

    protected DefaultListModel<String> lMovie;
    protected DefaultListModel<String> lHour;

    protected int space = 220;


    public ReservationFrame(){
        setResizable(false);
        setTitle("Rezervare bilete la film!");

        //edited/original text labels
        this.panelMsg = new JPanel();
        this.nameLabel.setText("Introducere nume: ");
        this.movieLabel.setText("Movie list: ");
        this.hoursLabel.setText("Hours list: ");
        this.nameLabel.setPreferredSize(new Dimension(space, 20));
        this.movieLabel.setPreferredSize(new Dimension(space, 20));
        this.hoursLabel.setPreferredSize(new Dimension(space, 20));

        this.panelMsg.add(Box.createHorizontalStrut(90));
        this.panelMsg.add(nameLabel);
        this.panelMsg.add(movieLabel);
        this.panelMsg.add(hoursLabel);

        this.p2 = new JPanel(new FlowLayout(FlowLayout.LEFT));
        this.name.setPreferredSize(new Dimension(150,20));

        lMovie = new DefaultListModel<>();
        lHour = new DefaultListModel<>();
        lMovie.addElement("Item1");
        lMovie.addElement("Item2");
        lMovie.addElement("Item3");
        lMovie.addElement("Item4");
        lHour.addElement("Item1");
        lHour.addElement("Item2");
        lHour.addElement("Item3");
        lHour.addElement("Item4");

        this.movieList = new JList<>(lMovie);
        movieList.setBounds(100,100, 75,75);
        movieList.getSelectionModel().addListSelectionListener(e->{
            String s;
            if(!e.getValueIsAdjusting()) {
                s = movieList.getSelectedValue();
                movieChoice = s;
                showHour(s);
            }
        });

        this.hourList = new JList<>(lHour);
        hourList.getSelectionModel().addListSelectionListener(e->{
            String s;
            if(!e.getValueIsAdjusting()) {
                s = hourList.getSelectedValue();
                hourChoice = s;
                DocumentManger.getInstance().setHourChoice(hourChoice);
            }
        });

        hourList.setBounds(100,100, 75,75);

        this.p2.add(Box.createHorizontalStrut(60));
        this.p2.add(name);
        this.p2.add(Box.createHorizontalStrut(40));
        this.p2.add(movieList);
        this.p2.add(Box.createHorizontalStrut(100));
        this.p2.add(hourList);

        //message field
        this.p3=new JPanel();
        this.msgField.setPreferredSize(new Dimension(700, 25));
        this.msgField.setEditable(false);
        this.p3=new JPanel();
        this.p3.setBorder(new EmptyBorder(5, 5, 5, 5));
        this.p3.add(msgField);

        //button field
        this.p4 = new JPanel();
        this.reservationBtn = new ReservationBtn(movieList, hourList, name, msgField, portNumber);
        this.reservationBtn.addActionListener(this);
        this.refreshBtn = new RefreshBtn(portNumber, msgField , lMovie);
        this.refreshBtn.addActionListener(this);
        this.p4.add(reservationBtn);
        this.p4.add(refreshBtn);

        //add all panels to pAll
        this.pAll=new JPanel();
        this.pAll.add(panelMsg);
        this.pAll.add(p2);
        this.pAll.add(p3);
        this.pAll.add(p4);

        //set y axis box layout and than pack the GUI
        BoxLayout layout = new BoxLayout(pAll, BoxLayout.Y_AXIS); //structurare layout
        this.pAll.setLayout(layout);
        this.add(pAll);
        this.setSize(800,600);
        super.setVisible(true);
        super.setDefaultCloseOperation(EXIT_ON_CLOSE);
    }

    @Override
    public void actionPerformed(ActionEvent e) {
        ((Command)e.getSource()).execute();
    }

    public void showHour(String movie){
        Socket socket = null;
        byte[] buffer = new byte[1024];
        int read;
        String s1 = "";
        String choice = "";
        String str = "3";
        s1 = movie.substring(0,2);
        if(Integer.parseInt(s1.trim()) < 10){
            choice+="0";
            choice+=s1;
        }else{
            choice+=s1;
        }
        str+=choice;
        DocumentManger.getInstance().setMovieChoice(choice);

        try {
            socket = new Socket("localhost", this.portNumber);
            socket.getOutputStream().write(str.getBytes("US-ASCII"));
            read = socket.getInputStream().read(buffer);
            if(read !=-1){
                String output = new String(buffer, 0, read);
                parseStringHour(output);
            }
            socket.close();
        } catch (UnknownHostException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void parseStringHour(String str) {
        String[] strings = str.split("\n");
        this.lHour.clear();
        for(String s : strings){
            lHour.addElement(s);
        }
    }
}
