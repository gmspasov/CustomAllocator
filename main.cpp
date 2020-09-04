#include <chrono>
#include <iostream>
#include "CustomAllocator.h"
using namespace std;
using namespace std::chrono;

int main()
{
    cout<<"Check if the allocator works correctly: "<<endl;
    size_t v;
    cout<<"The size must be larger or equal to 2^14 in order to be reasonable to use buddy allocator."
        <<endl<<"Enter the allocator size/(2^14): ";
    cin>>v;
    cout<<endl;
    CustomAllocator a (v*1024*16);

    //variables for the checks
    size_t numberOfAllocations=1;
    size_t sizeOfAllocations;
    size_t numberOfDeallocations=1;
    size_t choice=0;
    size_t indexWhereInTheArrayToPutTheNewPointer=0;
    size_t lastDeallocatedElement=0;
    size_t z=a.lastIndex();
    void**allocations=new void*[z+1];
    size_t numberOfFreeAndWhole=0;
    size_t numberOfAllocatedAndSplit=0;
    size_t numberOfAllocatedAndWhole=0;
    size_t sumOfSizeFreeAndWhole=0;
    size_t sumOfSizeAllocatedAndSplit=0;
    size_t sumOfSizeAllocatedAndWhole=0;

    //checks
    for(size_t k=0; k<=z; ++k)
    {
        if(a.isAllocated(k))
        {
            if(a.isSplit(k))
            {
                numberOfAllocatedAndSplit++;
                sumOfSizeAllocatedAndSplit+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                // cout<<"block number "<<k<<" is allocated and split "<<endl;
            }
            else
            {
                numberOfAllocatedAndWhole++;
                sumOfSizeAllocatedAndWhole+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                //cout<<"block number "<<k<<" is allocated and whole "<<endl;
            }
        }
        else
        {
            if(a.isSplit(k))
            {
                //cout<<"block number "<<k<<" is free and split "<<endl;
            }
            else
            {
                numberOfFreeAndWhole++;
                sumOfSizeFreeAndWhole+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                //cout<<"block number "<<k<<" is free and whole "<<endl;
            }
        }
    }

    cout<<"There are "<<numberOfFreeAndWhole<<" free and whole blocks after construction."<<endl;
    cout<<"With sum of sizes: "<<sumOfSizeFreeAndWhole<<" bytes. "<<endl;
    cout<<"There are " <<numberOfAllocatedAndSplit<<" allocated and split blocks after construction. "<<endl;
    cout<<"With sum of sizes: "<<sumOfSizeAllocatedAndSplit<<" bytes. "<<endl;
    cout<<"There are "<<numberOfAllocatedAndWhole<<" allocated and whole blocks after construction. "<<endl;
    cout<<"With sum of sizes: "<<sumOfSizeAllocatedAndWhole<<" bytes. "<<endl<<endl<<endl;

    cout<<"Lets check allocate and deallocate: "<<endl<<endl<<endl;


    while(true)
    {
        numberOfFreeAndWhole=0;
        numberOfAllocatedAndSplit=0;
        numberOfAllocatedAndWhole=0;
        sumOfSizeFreeAndWhole=0;
        sumOfSizeAllocatedAndSplit=0;
        sumOfSizeAllocatedAndWhole=0;
        if(lastDeallocatedElement>=z)
            lastDeallocatedElement=0;

        cout<<endl<<"What do you want to do? Enter 0 to end the check, 1 for allocations, 2 for deallocations"
            <<endl<<"and everything larger for the results until now. "<<endl
            <<"Your choice is: ";
        cin>>choice;
        cout<<endl;

        if(!choice)
        {
            cout<<endl<<"Are you sure you want to end the check? Confirm by entering zero, everything else means no: ";
            cin>>choice;
            cout<<endl;
            if(!choice)
                break;
            continue;
        }

        else  if(choice==1)
        {
            cout<<"Lets do some allocations. Enter the number of allocations that you want to make"
                <<endl<<" (if you don't want to allocate enter 0). Number of allocations = ";
            cin>>numberOfAllocations;
            cout<<endl;
            if(!numberOfAllocations)
                continue;

            if(numberOfAllocations+indexWhereInTheArrayToPutTheNewPointer>z)
            {
                cout<<"There are not so many free blocks. There are "<<z+1-indexWhereInTheArrayToPutTheNewPointer
                    <<" free blocks. "<<endl<<  "Returning to the choice menu"<<endl;
                continue;
            }

            cout<<endl<<"Now enter the size of the allocations. Size of allocations = ";
            cin>>sizeOfAllocations;
            for(size_t d=0; d<numberOfAllocations; ++d)
            {
                allocations[indexWhereInTheArrayToPutTheNewPointer+d]=a.allocate(sizeOfAllocations);
            }
            if(indexWhereInTheArrayToPutTheNewPointer+numberOfAllocations>=z)
                indexWhereInTheArrayToPutTheNewPointer=numberOfAllocations-(z-indexWhereInTheArrayToPutTheNewPointer)-1;
            else
                indexWhereInTheArrayToPutTheNewPointer+=numberOfAllocations;
        }
        else if(choice==2)
        {
            cout<<"Lets do some deallocations. Enter the number of deallocations that you want to make"
                <<endl<<" (if you don't want to deallocate enter 0). The program will try to the deallocate"
                <<endl<<" the blocks that were not deallocated until now. Number of deallocations = ";
            cin>>numberOfDeallocations;
            cout<<endl;

            if(!numberOfDeallocations)
                continue;

            if(numberOfDeallocations>=(z+1)/2)
                numberOfDeallocations=z/2;



            while(lastDeallocatedElement+numberOfDeallocations>z)
            {
                size_t numberOfDeallocationsLoop=z-lastDeallocatedElement;
                for(size_t d=0; d<numberOfDeallocationsLoop; ++d)
                    a.deallocate(allocations[lastDeallocatedElement+d]);
                lastDeallocatedElement=0;
                numberOfDeallocations-=numberOfDeallocationsLoop;
            }

            for(size_t d=0; d<numberOfDeallocations; ++d)
                a.deallocate(allocations[lastDeallocatedElement+d]);

            lastDeallocatedElement+=numberOfDeallocations;
        }

        else
        {
            for(size_t k=0; k<=z; ++k)
            {
                if(a.isAllocated(k))
                {
                    if(a.isSplit(k))
                    {
                        numberOfAllocatedAndSplit++;
                        sumOfSizeAllocatedAndSplit+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                        // cout<<"block number "<<k<<" is allocated and split "<<endl;
                    }
                    else
                    {
                        numberOfAllocatedAndWhole++;
                        sumOfSizeAllocatedAndWhole+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                        //  cout<<"block number "<<k<<" is allocated and whole "<<endl;
                    }
                }
                else
                {
                    if(a.isSplit(k))
                    {
                        //    cout<<"block number "<<k<<" is free and split "<<endl;
                    }
                    else
                    {
                        numberOfFreeAndWhole++;
                        sumOfSizeFreeAndWhole+=a.powerOfTwo(a.wholeBlockSizeExponent()-a.get_closestLargerExponent(k+2)+1);
                        //    cout<<"block number "<<k<<" is free and whole "<<endl;
                    }
                }
            }

            cout<<"There are "<<numberOfFreeAndWhole<<" free and whole blocks after check."<<endl;
            cout<<"With sum of sizes: "<<sumOfSizeFreeAndWhole<<" bytes. "<<endl;
            cout<<"There are " <<numberOfAllocatedAndSplit<<" allocated and split blocks after check. "<<endl;
            cout<<"With sum of sizes: "<<sumOfSizeAllocatedAndSplit<<" bytes. "<<endl;
            cout<<"There are "<<numberOfAllocatedAndWhole<<" allocated and whole blocks after check. "<<endl;
            cout<<"With sum of sizes: "<<sumOfSizeAllocatedAndWhole<<" bytes. "<<endl;
        }
    }
    for(size_t j=0; j<numberOfAllocations; ++j)
    {
        a.deallocate(allocations[j]);
    }
    cout<<endl<<"End of the check. Enter random number to continue with the performance check. "<<endl;
    int d;
    cin>>d;

    cout<<endl<<endl<<"Lets check performance for allocation and deallocation: "<<endl<<endl<<endl;
    while(true)
    {
        cout<<"Continue with the performance check? For no, enter 0, and for yes, enter larger number. Your choice: "<<endl;
        cin>>choice;

        if(!choice)
        {
            cout<<"Are you sure you want to end the performance check? For yes, enter 0, and for no, enter larger number. Your choice: "<<endl;
            cin>>choice;
            if(!choice)
                break;
            else
                continue;
        }
        else
        {
            cout<<"How many allocations/deallocations should be done? Number of allocations/deallocations = ";
            cin>>numberOfAllocations;
            cout<<endl;
            cout<<"How big allocations should be done? Size of allocations = ";
            cin>>sizeOfAllocations;
            cout<<endl;
            high_resolution_clock::time_point t1 = high_resolution_clock::now();
            for(size_t f=0; f<numberOfAllocations; ++f)
            {
                a.deallocate(a.allocate(sizeOfAllocations));
            }
            high_resolution_clock::time_point t2 = high_resolution_clock::now();
            duration<long long,std::nano> customAllocatorTime = duration_cast<duration<long long,std::nano>>(t2 - t1);

            t1=high_resolution_clock::now();
            for(size_t q=0; q<numberOfAllocations; ++q)
            {
                free(malloc(sizeOfAllocations));
            }
            t2=high_resolution_clock::now();
            duration<long long,std::nano> freeMallocTime = duration_cast<duration<long long,std::nano>>(t2 - t1);

            t1=high_resolution_clock::now();
            for(size_t s=0; s<numberOfAllocations; ++s)
            {
                delete(new char[sizeOfAllocations]);
            }
            t2=high_resolution_clock::now();

            duration<long long,std::nano> deleteNewTime = duration_cast<duration<long long,std::nano>>(t2 - t1);
            cout<<"The time with the custom is  "<<(customAllocatorTime.count())<<endl;
            cout<<"The time with free+malloc is "<<(freeMallocTime.count())<<endl;
            cout<<"The time with delete+new is  "<<(deleteNewTime.count())<<endl;
        }
    }
}
