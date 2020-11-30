package com.company;

public class DocumentManger {
    private static DocumentManger instance = null;
    protected String movieChoice;
    protected String hourChoice;

    public String getMovieChoice() {
        return movieChoice;
    }

    public void setMovieChoice(String movieChoice) {
        this.movieChoice = movieChoice;
    }

    public String getHourChoice() {
        return hourChoice;
    }

    public void setHourChoice(String hourChoice) {
        this.hourChoice = hourChoice;
    }

    private DocumentManger(){
    }

    public static DocumentManger getInstance(){
        if(instance == null){
            instance = new DocumentManger();
        }
        return instance;
    }
}
