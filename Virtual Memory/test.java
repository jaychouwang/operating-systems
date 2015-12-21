public class test{

public static void main(String [] args){

	int counter = 0;
	
	System.out.println("Counter: " + counter);
	
	int mask = 128;
	counter = (counter | mask);
	
	System.out.println("Counter: " + counter);
	
	counter = (counter >>> 1);
	
	System.out.println("Counter: " + counter);
	
	counter = (counter >>> 1);
	counter = (counter | mask);
	
	System.out.println("Counter: " + counter);

}

}