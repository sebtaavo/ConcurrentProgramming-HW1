package birds;

/*Class representing the consumer in the multithreading consumer-producer project.*/
public class BabyBird extends Thread{
    private WormDish dish;
    private volatile boolean stop = false; /*volatile for thread visibility i think*/
    private final int identifier;

    public BabyBird(WormDish dish, int identifier){
        this.dish = dish;
        this.identifier = identifier;
    }

    public void setStop(){
        this.stop = true;
        this.interrupt();
    }

    public void run(){
        while(!stop){
            dish.consumeWorms(this.identifier);
            try {
                Thread.sleep((int) (Math.random() * 2000)); /*simulate time to eat one worm*/
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
