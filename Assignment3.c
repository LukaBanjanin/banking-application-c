#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct _bank_account {//bank account struct with all relevant variables declared. To be populated while reading in the input file 
    char* name;
    char type;
    int depositFee;
    int withdrawFee;
    int transactionFee;
    int maxTransactions;
    int additionalFee;
    int overdraftFee;
    int overdraftApplied;//counts how many times we have applied overdraft
    int balance;
    int transactionCount;
} BankAccount;

typedef struct _targs {//thread struct with all relevant variables for threads and mutual exclusion to function properly in the code 
    char* buff;
    pthread_mutex_t *lock;
    BankAccount** accounts;
    int count;
} targs;

BankAccount *find_bank_account(BankAccount **accounts, int count, char * name) {//helper function created to make locating a specific bank account easier
    for(int i = 0; i < count; i++) {
        if(strcmp(accounts[i] -> name, name) == 0) return accounts[i];//returns pointer to a bank account struct, specificlly the one we are searching for
    }
    return NULL;
}

void depositors(char* buff, BankAccount** accounts, int count) {//function used to deal with all initial deposits to accounts before client threads start processing
    char* token = strtok (buff, " ");
    while ((token = strtok(NULL, " ")) != NULL) {
        token = strtok(NULL, " ");
        BankAccount* tmp = find_bank_account(accounts, count, token);//calls find bank account to get the pointer of the struct with the account we need
        if(tmp == NULL) {
            printf("Error couldn't find bank account!\n");
        }
        token = strtok(NULL, " ");//gets the amount to be deposited from the input file
        int amount = atoi(token);//sets amount variable equal to the amount to be deposited according to the input file
        tmp -> balance += amount;//updates the balance 
        tmp -> balance -= tmp -> depositFee;//accounts for any depoosit fee
        tmp -> transactionCount++;//increases the transaction count
    }
}

void* handle_clients(void* args) {//function used to handle the client threads once all accounts have been created and deposits have been deposited
    targs* arguments = (targs*)args;
    char* token = strtok(arguments -> buff, " ");
    
    while ((token = strtok(NULL, " ")) != NULL) {
        pthread_mutex_lock(arguments -> lock);//locks the critical section of the code
        if(strcmp(token, "d") == 0) {//checks if its a deposit
            token = strtok(NULL, " ");//moves the token to the account to recieve the deposit 
            BankAccount *tmp = find_bank_account(arguments -> accounts, arguments -> count, token);//gets a pointer to this account struct
            token = strtok(NULL, " ");//moves the token to the amount of the deposit
            int amount = atoi(token);

           
            int dfee = tmp -> depositFee;//gets the deposit fee if the acount has one
            amount -= dfee;//applies the deposit fee on the balance if that account has one

            if (tmp -> transactionCount >= tmp -> maxTransactions ) amount -= tmp -> additionalFee;//checks if the account is over its maximum allowable transactions and applies fees and updates transaction count accordingly
            tmp -> balance += amount; 
            tmp -> transactionCount += 1;
        }
        else if(strcmp(token, "w") == 0) {//same as outer if loop above except now doing a withdrawl instead of deposit because the token is a w and not a d

            token = strtok(NULL, " ");

            BankAccount *tmp = find_bank_account(arguments -> accounts, arguments -> count, token);
            token = strtok(NULL, " ");
            int amount = atoi(token);

            int wfee = tmp -> withdrawFee;
            tmp -> overdraftApplied += 0;

            if (tmp -> transactionCount >= tmp -> maxTransactions ) 
                wfee += tmp -> additionalFee;

            if (tmp -> balance - amount - wfee < 0 && tmp -> overdraftFee != -1) {//checks if we are at a negative balance and applies overdraft fee accordingly
                int ODcount = (tmp -> balance - amount - wfee) / 500;
                if (ODcount > tmp -> overdraftApplied) {
                
                div_t qt = div(tmp -> balance - amount - wfee, 500);
                wfee += (ODcount - tmp -> overdraftApplied) * qt.quot * tmp -> overdraftFee;
                tmp -> overdraftApplied = ODcount;

                }

                if(tmp -> balance - amount - wfee < -5000) {//doesnt allow a balnce to go more than -5000
                    pthread_mutex_unlock(arguments -> lock);//unlocks the critical section right before it leaves
                    continue;
                }
                
            }
            else if(tmp -> balance - amount - wfee < 0 && tmp -> overdraftFee == -1) {//doesnt allow accounts without overdraft protection to go into a negative balance
                pthread_mutex_unlock(arguments -> lock);//unlocks the critical section right before it leaves 
                continue; 
            }
            tmp -> balance = tmp -> balance - amount - wfee;
            tmp -> transactionCount += 1;
        }
        else if(strcmp(token, "t") == 0) {//the same as the above 2 outer loops that check if the token is d or w except this is a transfer so now we transfer funds from one account to another and apply fees to each according to their specific account details found in their bank account struct 


            token = strtok(NULL, " ");


            BankAccount *tmp1 = find_bank_account(arguments -> accounts, arguments -> count, token);
            token = strtok(NULL, " ");


            BankAccount *tmp2 = find_bank_account(arguments -> accounts, arguments -> count, token);
            token = strtok(NULL, " ");
            int amount = atoi(token);

            int tfee1 = tmp1 -> transactionFee;
            int tfee2 = tmp2 -> transactionFee;

            if (tmp1 -> transactionCount >= tmp1 -> maxTransactions ) 
                tfee1 += tmp1 -> additionalFee;
            
            if (tmp2 -> transactionCount >= tmp2 -> maxTransactions ) 
                tfee2 += tmp2 -> additionalFee;

            if (tmp1 -> balance - amount - tfee1 < 0 && tmp1 -> overdraftFee != -1) {

                int ODcount = (tmp1 -> balance - amount - tfee1) / 500;
                if (ODcount > tmp1 -> overdraftApplied) {
                div_t qt = div(tmp1 -> balance - amount - tfee1, 500);
                tfee1 += (ODcount - tmp1 -> overdraftApplied) * qt.quot * tmp1 -> overdraftFee;
                tmp1 -> overdraftApplied = ODcount;

                }

                if(tmp1 -> balance - amount - tfee1 < -5000) {
                    pthread_mutex_unlock(arguments -> lock);
                    continue;
                }
            }
            else if(tmp1 -> balance - amount - tfee1 < 0 && tmp1 -> overdraftFee == -1) {
                pthread_mutex_unlock(arguments -> lock);
                continue; 
            }

            tmp1 -> balance = tmp1 -> balance - amount - tfee1;
            tmp2 -> balance = tmp2 -> balance + amount - tfee2;
            tmp1 -> transactionCount += 1;
            tmp2 -> transactionCount += 1; 

            
        }
        pthread_mutex_unlock(arguments -> lock);
    }
    
}

BankAccount* create_bank_account (char* buff) {//function that creates the bank account struct for each account in the input file and initializes all of the variables according to the input files specified details
    BankAccount* ret = (BankAccount*) malloc(sizeof(BankAccount));
    ret -> balance = 0;
    ret -> transactionCount = 0;
    char* token = strtok (buff, " ");
    ret -> name = malloc(strlen(token) + 1);
    strcpy(ret -> name, token);

    while ((token = strtok(NULL, " ")) != NULL) {//while loop that parses to line in an input file
        if (strcmp(token, "type") == 0) {//sets the type of the account to either business or personal according to the input file
            token = strtok(NULL, " ");
            if (strcmp(token, "business") == 0) {
                ret -> type = 'b';//business account
            }
            else 
                ret -> type = 'p';//personal account
        }
        else if (strcmp(token, "d") == 0) {//initializes deposit fee for this specific account
            token = strtok(NULL, " ");
            ret -> depositFee = atoi(token);
        }
        else if (strcmp(token, "w") == 0) {//initializes withdraw fee for this specific account
            token = strtok(NULL, " ");
            ret -> withdrawFee = atoi(token);
        }
        else if (strcmp(token, "t") == 0) {//initializes the transfer fee
            token = strtok(NULL, " ");
            ret -> transactionFee = atoi(token);
        }
        else if (strcmp(token, "transactions") == 0) {
            token = strtok(NULL, " ");
            ret -> maxTransactions = atoi(token);//initializes the maximum number of transactions that account is permited to do before a fee starts being applied for each additional transaction after that
            token = strtok(NULL, " ");
            ret -> additionalFee = atoi(token);//initializes how much the fee is for that account for each additional transaction over its allowed maximum number of transactions
        }
        else if (strcmp(token, "overdraft") == 0) {//if the account has ODP, initializes it according to the amount in the input file
            token = strtok(NULL, " ");
            if (strcmp(token, "Y") == 0) {
                token = strtok(NULL, " ");
                ret -> overdraftFee = atoi(token);
            }
            else ret -> overdraftFee = -1;//sets ODP fee to -1 if the account doesn't have ODP
        }
    }
    return ret;
}

int main() {

    FILE* infile = fopen("assignment_3_input_file.txt", "r");//opens the input file for reading
    char buff[1024] = {0};

    BankAccount* b_array[10] = {0};//pointer to store all of the bank accounts we create
    int count = 0;


    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);//intitializes the thread locking to ensure mutual exclusion amongst threads when entering critical sections of code
    
    pthread_t id[20] = {0};
    int tid = 0;
    while (fgets(buff, 1024, infile) != NULL) {//while loop that parses the input file
        if (buff[0] == 'a'){//if the line starts with a then it is the account descriptions so call the create bank account function to make the bank account structs
            b_array[count] = create_bank_account(buff);
            count++;
        }
        else if (buff[0] == 'd') {//if the line starts with a d then it is the initial depositors so call the depositors function to deposit funds in the account
            depositors(buff, b_array, count);
        }
          
        else {
            targs* tmp = (targs*)malloc(sizeof(targs));//if it is neither of the above then it has to start with c and therefore be the client transactions
            tmp -> buff = malloc(strlen(buff) + 1);//allocate enough memory for the full buffer string plus 1 for the end of string terminator
            strcpy(tmp -> buff, buff);//copy the buffer over to the one in the targs struct

            tmp -> accounts = b_array;//initialize the variables in targs struct as follows
            tmp -> lock = &lock;
            tmp -> count = count;
            pthread_create(&id[tid], NULL, (handle_clients), (void*) tmp);//create threads so all clients can be processed simultaneously
            tid += 1;//increase thread id count everytime a thread is made
        }
    }
     for(int k = 0; k < tid; k++) {//join all of the threads when the above outer loop is finished accoding to their thead id's
         pthread_join(id[k], NULL);
    }

    FILE *outfile = fopen("assignment_3_output_file.txt", "w");//open a text file so we can write the final balances of the accounts
    for(int i = 0; i < count; i++) {//looping through each of the bank account structs
        char type[128];
        if(b_array[i] -> type == 'p') strcpy(type, "personal");
        else strcpy(type, "business");
        fprintf(outfile, "%s type %s %d\n", b_array[i] -> name, type, b_array[i] -> balance);//printing each accounts info to the file we opened above
        printf("%s type %s %d\n", b_array[i] -> name, type, b_array[i] -> balance);//printing each accounts info to the screen
    }

    fclose(infile);//close the input file we were reading from
    fclose(outfile);//close the output file we were writing to

    return 0;
}