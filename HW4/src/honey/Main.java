package honey;

import java.util.ArrayList;

public class Main {
    public static void main(String[] args) {
        System.out.println("Starting program...");
        int numberOfBees = 10;

        /*Init the honey pot, bees and bear*/
        HoneyPot pot = new HoneyPot(20);
        ArrayList<Bee> bees = new ArrayList<Bee>();
        for(int i = 0; i < numberOfBees; i++){
            bees.add(new Bee(pot, i + 1));
        }
        Bear bear = new Bear(pot);

        /*start the threads*/
        bear.start();
        for(int i = 0; i < numberOfBees; i++){
            bees.get(i).start();
        }

        /*run a set amount of time before closing*/
        try {
            int seconds = 20;
            Thread.sleep(seconds * 1000); /*takes in milliseconds*/
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        /*call stop on the threads so they close their loops*/
        bear.setStop();
        for (Bee bee : bees) {
            bee.setStop();
        }

        /*wait for all threads to finish by calling join*/
        try {
            bear.join();
            for (Bee bee : bees) {
                bee.join();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}