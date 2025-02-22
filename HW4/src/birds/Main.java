package birds;

import java.util.ArrayList;

public class Main {
    public static void main(String[] args) {
        System.out.println("Starting program...");
        int numberOfBabyBirds = 10;

        /*Init the worm dish, baby birds and mama bird*/
        WormDish dish = new WormDish(20);
        ArrayList<BabyBird> babyBirds = new ArrayList<BabyBird>();
        for(int i = 0; i < numberOfBabyBirds; i++){
            babyBirds.add(new BabyBird(dish, i + 1));
        }
        MamaBird mamaBird = new MamaBird(dish);

        /*start the threads*/
        mamaBird.start();
        for(int i = 0; i < numberOfBabyBirds; i++){
            babyBirds.get(i).start();
        }

        /*run a set amount of time before closing*/
        try {
            int seconds = 20;
            Thread.sleep(seconds * 1000); /*takes in milliseconds*/
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        /*call stop on the threads so they close their loops*/
        mamaBird.setStop();
        for (BabyBird babyBird : babyBirds) {
            babyBird.setStop();
        }

        /*wait for all threads to finish by calling join*/
        try {
            mamaBird.join();
            for (BabyBird babyBird : babyBirds) {
                babyBird.join();
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}