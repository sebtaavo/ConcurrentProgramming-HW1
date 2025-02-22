package birds;

/*Intended to be a monitor class for the shared resource in the producer-consumer multithreading problem
 * involving a mama bird and many baby birds.*/

public class WormDish {
    private final int maxCapacity;
    private int currentWorms;
    private boolean emptyDish;

    public WormDish(int maxCapacity){
        this.maxCapacity = maxCapacity;
        this.currentWorms = this.maxCapacity;
        this.emptyDish = false;
    }

    public synchronized void depositWorms(){
        while (!emptyDish){
            try{
                wait();
            } catch (InterruptedException e){
                Thread.currentThread().interrupt();
                return;
            }
        }
        this.currentWorms = this.maxCapacity;
        System.out.printf("Mama bird deposits %d worms.\n", this.maxCapacity);
        this.emptyDish = false;
        notifyAll();
        System.out.println("Mama bird lets sleeping threads know there is work.");
    }/*end of deposit worms method*/

    public synchronized void consumeWorms(int identifier){
        while(emptyDish){
            try{
                wait();
            } catch (InterruptedException e){
                Thread.currentThread().interrupt();
                return;
            }
        }
        System.out.printf("Baby bird number %d consumes a worm\n", identifier);
        this.currentWorms--;
        if(currentWorms <= 0){
            emptyDish = true;
            notifyAll();
            System.out.printf("Baby bird number %d notifies mama bird that dish is empty.\n", identifier);
        }
    }/*end of consuming worms method*/
}
