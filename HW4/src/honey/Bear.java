package honey;

/*Class representing the consumer in the multithreading consumer-producer project.*/
public class Bear extends Thread{
    private HoneyPot pot;
    private volatile boolean stop = false; /*volatile for thread visibility i think*/

    public Bear(HoneyPot pot){
        this.pot = pot;
    }

    public void setStop(){
        this.stop = true;
        this.interrupt();
    }

    public void run(){
        while(!stop){
            pot.consumeAllHoney();
            try {
                Thread.sleep(1000); /*Simulate time to eat honey*/
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
