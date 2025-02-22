package honey;

/*Intended to be a monitor class for the shared resource in the producer-consumer multithreading problem
* involving a consuming bear and multiple honeybees.*/

public class HoneyPot {
    private final int maxCapacity;
    private int emptySlots;
    private boolean fullPot;

    public HoneyPot(int maxCapacity){
        this.maxCapacity = maxCapacity;
        this.emptySlots = this.maxCapacity;
        this.fullPot = false;
    }

    public synchronized void depositHoney(int identifier){
        while (fullPot){
            try{
                wait();
            } catch (InterruptedException e){
                Thread.currentThread().interrupt();
                return;
            }
        }
        emptySlots--;
        System.out.printf("Honey was deposited by bee number %d \n", identifier);
        System.out.printf("Amount of honey now: %d / %d\n", (this.maxCapacity - this.emptySlots), this.maxCapacity);
        if(emptySlots <= 0){
            fullPot = true;
            notifyAll();
            System.out.printf("Bee number %d notifies all threads that the pot is full\n", identifier);
        }
    }/*end of deposit honey method*/

    public synchronized void consumeAllHoney(){
        while(!fullPot){
            try{
                wait();
            } catch (InterruptedException e){
                Thread.currentThread().interrupt();
                return;
            }
        }
        this.emptySlots = this.maxCapacity; /*we empty the honey pot*/
        System.out.println("The bear consumes all the honey and lets the bees know they can get working again.");
        fullPot = false;
        notifyAll();
    }/*end of consuming honey method*/
}
