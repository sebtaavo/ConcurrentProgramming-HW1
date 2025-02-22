package birds;

/*Class representing the producer in the multithreading producer-consumer problem involving mama bird and baby birds.*/
public class MamaBird extends Thread{
    private final WormDish dish;
    private volatile boolean stop = false; /*volatile for thread visibility i think*/

    public MamaBird(WormDish dish){
        this.dish = dish;
    }

    public void setStop(){
        this.stop = true;
        this.interrupt();
    }

    public void run(){
        while(!stop){
            this.dish.depositWorms();
            try {
                Thread.sleep((int) (Math.random() * 2000)); /*simulate time to fetch more worms*/
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
