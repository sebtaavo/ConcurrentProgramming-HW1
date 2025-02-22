package honey;

/*Class representing the producer in the multithreading producer-consumer problem involving honeybees, bears and a honey pot.
* At any given run, there are many (>1) bee threads active.*/
public class Bee extends Thread{
    private final HoneyPot pot;
    private volatile boolean stop = false; /*volatile for thread visibility i think*/
    private final int identifier;

    public Bee(HoneyPot pot, int identifier){
        this.pot = pot;
        this.identifier = identifier;
    }

    public void setStop(){
        this.stop = true;
        this.interrupt();
    }

    public void run(){
        while(!stop){
            this.pot.depositHoney(this.identifier);
            try {
                Thread.sleep((int) (Math.random() * 2000)); /*simulate time to fetch honey*/
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
