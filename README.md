# Κ24 - Προγραμματισμός Συστήματος - **2**η Εργασία, Εαρινό Εξάμηνο 2024 

<h1> jobExecutor: a Multithreaded Network Application</h1>

*από τον Κωνσταντίνο Αρκουλή sdi2100008*

## Περιεχόμενα

- [Makefile και ενδεικτικές εκτελέσεις](#makefile-και-ενδεικτικές-εκτελέσεις)
- [Συνοπτική επεξήγηση αρχείων πηγαίου κώδικα](#συνοπτική-επεξήγηση-αρχείων-πηγαίου-κώδικα)
- [Συγχρονισμός νημάτων controllers-workers](#συγχρονισμός-νημάτων-controllers-workers)
- [Συγχρονισμός κατά το exit του server](#συγχρονισμός-κατά-το-exit-του-server)
- [Παραδοχές, διευκρινίσεις και δυνατότητες](#παραδοχές-διευκρινίσεις-και-δυνατότητες)

## Makefile και ενδεικτικές εκτελέσεις

- To παρόν `Makefile` υποστηρίζει separate compilation όπως και ζητείται εξάλλου.
- Όλα τα παρακάτω make rules θα πρέπει να εφαρμοστούν στο επίπεδο του parental folder, δηλαδή σε αυτό που περιέχει το `Makefile`.

<h4> Make rules </h4>

- `make`: Μεταγλώττιση κώδικα.
- `make clean`: Καθαρισμός εκτελέσιμων και αντικειμενικών αρχείων.
- `make deep_clean`: Καθαρισμός όλων των αρχείων που βρίσκονται στα directories:
    1. `/bin`
    2. `/build`
    3. `/errors`
    4. `/output`
    5. `/valgrind` 

    επιπλέον διαγράφονται τελείως τα directories 3, 4 και 5 της παραπάνω λίστας.
- `make rtest`: Μεταγλώττιση κώδικα και τρέξιμο ενός συγκεκριμένου test από τα test του directory `/tests` (παραδείγματα παρακάτω).

- `make run` και `make val`: Μεταγλώττιση κώδικα και τρέξιμο jobExecutorServer με παραμέτρους που αρχικοποιούνται στο ίδιο το `Makefile`. Με το `make val` επιπλέον, εκτελείται ο server με το valgrind για έλεγχο των memory leaks (τοποθέτηση του valgrind output σε ένα log file στο `/valgrind`).

Για τον έλεγχο της ορθότητας της υλοποίησης αυτής, ο χρήστης έχει τρεις επιλογές:
1. Έχοντας κάνει make, τρέχει manually τον jobExecutorServer και μετά manually τους commanders που θέλει, ακολουθώντας τις υποδείξεις της εκφώνησης για τα arguments. Παράδειγμα:

```
$ make
mkdir -p ./bin
mkdir -p ./build
mkdir -p ./output
mkdir -p ./errors
mkdir -p ./valgrind
gcc -c src/jobCommander.c -o build/jobCommander.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/processUtils.c -o build/processUtils.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/parsingUtils.c -o build/parsingUtils.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/jobQueue.c -o build/jobQueue.o -Wall -Wextra -Werror -g -pthread -I./include
gcc build/jobCommander.o build/processUtils.o build/parsingUtils.o build/jobQueue.o -o bin/jobCommander -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc -c src/jobExecutorServer.c -o build/jobExecutorServer.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/controller.c -o build/controller.o -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc -c src/worker.c -o build/worker.o -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc build/jobExecutorServer.o build/processUtils.o build/parsingUtils.o build/jobQueue.o build/controller.o build/worker.o -o bin/jobExecutorServer -Wall -Wextra -Werror -g -pthread -I./include -lpthread

$ ./bin/jobExecutorServer 5555 8 4 &
[1] 1283298

$ ./bin/jobCommander linux13.di.uoa.gr 5555 issueJob ls
JOB < job_1, ls > SUBMITTED

-----job_1 output start------
bin
build
count.sh
errors
include
kill_zombies.sh
Makefile
output
README.md
src
syspro2024_hw2_completion_report.pdf
tests
valgrind
-----job_1 output end------

$ ./bin/jobCommander linux13.di.uoa.gr 5555 exit
SERVER TERMINATED
Server exiting...
[1]+  Done                    ./bin/jobExecutorServer 5555 8 4
```

2. Τρέχει το `make rtest` make rule για να τρέξει ένα test της επιλογής του (από αυτά που βρίσκονται στο directory `/tests`) αφού έχει επιλέξει τα arguments που θέλει στην αρχή του αντίστοιχου `test_jobExecutor_*.sh` αρχείου. Παράδειγμα:


```
$ make rtest TEST=test_jobExecutor_1.sh
mkdir -p ./bin
mkdir -p ./build
mkdir -p ./output
mkdir -p ./errors
mkdir -p ./valgrind
gcc -c src/jobCommander.c -o build/jobCommander.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/processUtils.c -o build/processUtils.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/parsingUtils.c -o build/parsingUtils.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/jobQueue.c -o build/jobQueue.o -Wall -Wextra -Werror -g -pthread -I./include
gcc build/jobCommander.o build/processUtils.o build/parsingUtils.o build/jobQueue.o -o bin/jobCommander -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc -c src/jobExecutorServer.c -o build/jobExecutorServer.o -Wall -Wextra -Werror -g -pthread -I./include
gcc -c src/controller.c -o build/controller.o -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc -c src/worker.c -o build/worker.o -Wall -Wextra -Werror -g -pthread -I./include -lpthread
gcc build/jobExecutorServer.o build/processUtils.o build/parsingUtils.o build/jobQueue.o build/controller.o build/worker.o -o bin/jobExecutorServer -Wall -Wextra -Werror -g -pthread -I./include -lpthread
./tests/test_jobExecutor_1.sh || true
JOB < job_1, touch myFile.txt > SUBMITTED

-----job_1 output start------
-----job_1 output end------

myFile.txt
JOB < job_2, rm myFile.txt > SUBMITTED

-----job_2 output start------
-----job_2 output end------

ls: cannot access 'myFile.txt': No such file or directory
SERVER TERMINATED
```

3. Τρέχει `make run` (ή `make val`) και μετά κάποιους από τους έτοιμους jobCommanders που βρίσκονται στο `Makefile` μέσω των make rules: `make cmdr_prog`, `make cmdr_ls` και `make cmdr_exit` (βλ. `Makefile`), αφού έχει επιλέξει τα επιθυμητά arguments στην αρχή του `Makefile`.

## Συνοπτική επεξήγηση αρχείων πηγαίου κώδικα
Για να διατηρηθεί το παρόν `README.md`, μικρό σε μέγεθος και σχετικά ευανάγνωστο, δεν θα μπούμε σε πολλές λεπτομέρειες για τον ίδιο τον κώδικα. Τα comments του κώδικα καλύπτουν αυτό το κομμάτι και με το παραπάνω. Παρόλ' αυτά, συνοπτικά (για λόγους indexing) έχουμε:

- `controller.c` (header αρχείο `controller.h`): περιέχεται ο σχετικός κώδικας με τα controller threads.
- `jobCommander.c`: περιέχεται ο σχετικός κώδικας με τους clients του application μας(jobCommanders processes).
- `jobExecutorServer.c`: το main thread του server.
- `jobQueue.c` (header αρχείο `jobQueue.h`): το module της ουράς στατικού max μεγέθους που χρησιμοποιείται στην θέση του buffer των εργασιών σε αναμονή.
- `parsingUtils.c` (header αρχείο `parsingUtils.h`): το module με βοηθητικές συναρτήσεις για το parsing των strings.
- `processUtils.c` (header αρχείο `processUtils.h`): το module με βοηθητικές συναρτήσεις για τα processes, κυρίως της μορφής wrapper functions.
- `worker.c` (header αρχείο `worker.h`): περιέχεται ο σχετικός κώδικας με τα worker threads.
- Στο αρχείο επικεφαλίδας `common.h` βρίσκονται οι κοινές δομές και τα libraries που χρησιμοποιούνται σχεδόν σε όλα τα `.c` αρχεία. Επιπλέον εκεί βρίσκονται διάφορα macros για μερικές συναρτήσεις της `pthreads.h` όπως για παράδειγμα το macro `LOCK` που περιέχει την `pthread_mutex_lock`. Ο χρήστης ενθαρρύνεται να ρίξει μια ματιά στο παρόν αρχείο για την ουσιαστική κατανόηση του κώδικα και των σχημάτων συγχρονισμού που ακολουθούν.

## Συγχρονισμός νημάτων controllers-workers

Το πρόβλημα συγχρονισμού μεταξύ των controllers-workers είναι παρόμοιο με αυτό των producers-consumers όπου ο buffer είναι το στατικά μέγιστου μεγέθους `bufferQueue` μας (βλ. `jobExecutorServer.c`) με το επιπλέον constraint ότι ταυτόχρονα θα εκτελούνται το πολύ `concurrency` number of workers (όπου concurrency είναι ένας ακέραιος >= 1). Επομένως, η λογική που επέλεξα έχει ως εξής:

- Κάθε producer (controller thread) που είναι issueJob type (και άρα θέλει να κάνει enqueue ένα job στην στατικού max μεγέθους ουρά aka `bufferQueue`), λαμβάνει πρώτα το mutex που προστετεύει την ουρά και το `id` (το mutex αυτό είναι το `mtx`), δημιουργεί το καινούργιο job και αυξάνει το `id` κατά 1. Έπειτα, όσο η ουρά είναι γεμάτη και δεν έχει ξεκινήσει η διαδικασία εξόδου του server (`exit_initited` περισσότερα για αυτό στην επόμενη ενότητα), ο controller αυτός κοιμάται στο condition variable `full` αφήνοντας το `mtx`. Αλλιώς (ή όταν ξυπνήσει), τοποθετεί το καινούργιο job στην ουρά, κάνει ένα signal στο `empty_or_max_concur` condition variable των workers (εξήγηση ακριβώς από κάτω) σηματοδώντας ότι η ουρά σίγουρα δεν είναι πλέον άδεια, σε περίπτωση που ένας worker (consumer) κοιμάται λόγω αυτού, και στέλνει το 'JOB SUBMITTED' μήνυμα στον αντίστοιχο jobCommander. Επιπλέον, αφού περάσει τον έλεγχο για το `full` condition variable και πριν κάνει enqueue ένα job στην ουρά, ελέγχεται πάλι το `exit_initiated` flag σε περίπτωση που ο λόγος που ξύπνησε είναι για να τερματίσει ακολουθώντας το πρωτόκολλο που εξηγούμε στην επόμενη ενότητα. Τέλος, για τον συγχρονισμό των controllers μεταξύ τους όταν αυτοί μεταβάλλουν global δομες (π.χ. το `bufferQueue` που επηρεάζεται προφανώς και από τον stop type controller, το `mtx` μας προστατεύει από μόνο του, με εξαίρεση την κοινή μεταβλητή `active_controllers` που προστατεύεται από άλλο mutex και θα αναλυθεί και αυτό μαζί με τα υπόλοιπα στο [Συγχρονισμός κατά το exit του server](#συγχρονισμός-κατά-το-exit-του-server).

- Κάθε consumer (worker thread) βρίσκεται μόνιμα σε μια while(true) loop από την δημιουργία τους από το main thread του server μέχρι την έναρξη ρουτίνας τερματισμού του server. Μέσα σε αυτή, ο worker λαμβάνει το `mtx` και σε περίπτωση που η ουρά είναι άδεια ή έχουμε maximum concurrency, και δεν έχει γίνει set σε true το flag `exit_initiated`, τότε ο worker κοιμάται στο condition variable `empty_or_max_concur`, αφήνοντας πρώτα το `mtx`. Αλλιώς (ή όταν ξυπνήσει) εκτελεί τις εξής ενέργειες:

1. Αφαιρεί ένα job από την ουρά.
2. Κάνει signal στο condition variable `full`, σηματοδωτόντας ότι η ουρά σίγουρα δεν είναι πλέον άδεια σε περίπτωση που κάποιος controller (producer) κοιμάται λόγω αυτού.
3. Αυξάνει τον counter των ενεργών workers (`active_workers`) κατά 1, το οποίο επίσης προστατεύεται αποκλειστικά από το `mtx`.
4. Αφήνει το `mtx` και πλέον μπορεί να συνεχίσει τις παρακάτω ενέργειες χωρίς φόβο για τον συγχρονισμό του συστήματος.

    Ο worker τώρα μπορεί να εκτελέσει χρήσιμο έργο. Συγκεκριμένα, θέτει σε εκτέλεση το job που αφαιρέθηκε από την ουρά μέσω της χρήσης `fork`-`execvp`, έχοντας πρώτα κάνει `dup2` το `STDOUT_FILE` και `STDERR_FILENO` του child process της `fork` για να γραφούν αυτά τα output στο `/output/[child_pid].output` αρχείο.
    Έπειτα, περιμένει μέσω της `wait` την ολοκλήρωση του child process και μετά στέλνει το αποτέλεσμα (το αρχείο `/output/[child_pid].output`) στον αντίστοιχο jobCommander και καταστρέφει το job και το παραπάνω αρχείο, απελευθερώνοντας πόρους του συστήματος.
    Τέλος κατά την έξοδο των workers, πριν ξεκινήσει πάλι την επόμενη επανάληψη, ο worker λαμβάνει και πάλι το `mtx` για να μειώσει τον counter `active_workers` κατά 1.

Με αυτό το σχήμα συγχρονισμού που μόλις περιγράψαμε, το οποίο χρησιμοποιεί τα:
- mutexes: `mtx`
- condition variables: `full`, `empty_or_max_concur`
- global/κοινές δομές: `concurrency`, `bufferQueue`, `active_workers`

επιτυγχάνουμε την ορθή λειτουργία και συγχρονισμό του server κατά την πραγματοποίηση χρήσιμου έργου δηλαδή κατά την εξυπηρέτηση των requests των clients (jobCommanders) όλων των ειδών (πέραν του exit jobCommander που αναλύεται ακριβώς από κάτω) με τις προυποθέσεις που αναφέραμε πιο πάνω (concurrency control κτλ).

## Συγχρονισμός κατά το exit του server

- Αρχικά, όταν ο server ξεκινήσει την λειτουργία του και υπάρχει ακόμα μόνο ένα thread, το main thread, τότε αυτό κάνει spawn τα `threadPoolSize` αριθμό από worker threads και φυλά τις δομές `pthread_t` τους σε έναν μονοδιάστατο πίνακα. Στο τέλος της exit ρουτίνας που θα περιγράψουμε ευθύς αμέσως, το main thread αυτό (`jobExecutorServer.c`) έχοντας πλέον βγει από το while loop του, περιμένει πρώτα τους εναπομείναντες ενεργούς controllers (με το σχήμα συγχρονισμού που εξηγείται παρακάτω) και έπειτα καλείται η  `pthread_join` για κάθε `pthread_t` των workers του πίνακα που αναφέραμε προηγουμένως, έτσι ώστε να βεβαιωθούμε ότι τόσο τα controller threads όσο και τα worker threads θα έχουν τερματίσει και θα έχει ολοκληρώσει η `pthread_exit` τους, πριν το main thread προχωρήσει στο cleanup των resources και το τελικό `exit`.

- Όπως θα καταλάβετε και παρακάτω, το σχήμα συγχρονισμού για να περιμένει το main thread τα controller threads, δεν μπορεί να εγγυηθεί ότι οι δεύτεροι θα έχουν προλάβει να τρέξουν ΚΑΙ την `pthread_exit`, πράγμα όμως που δεν μας επηρρεάζει στην ορθή λειτουργία του server (ενδεχομένως δηλαδή να υπάρχει ένα μικρό 'αθώο' leak), καθώς οτιδήποτε πριν από αυτή την εντολή (στους controllers) θα έχει προλάβει να ολοκληρωθεί.

- Όπως αναφέρθηκε και προηγουμένως, ο server έχει μια global μεταβλητή flag που σηματοδοτεί ότι η ρουτίνα εξόδου του έχει ξεκινήσει, αυτή είναι η `exit_initiated`. Όταν ένας controller τύπου exit εμφανιστεί, τότε αυτός (ο controller), εκτελεί τις παρακάτω ενέργειες, έχοντας πρώτα πάρει το `mtx`:

    1. Κάνει ένα signal `SIGUSR1` μέσω της `pthread_kill` στο main thread, το οποίο γίνεται handle από το `sigusr1_handler` του main thread. Σε αυτόν τον signal handler γίνεται set σε true το flag `exit_initiated` και κλείνει το main server socket file descriptor από το οποίο ο server ακούει νέες συνδέσεις (`server_socketfd`), έτσι ώστε ο server να μην δεχτεί άλλους clients.

    2. Στέλνει το ενημερωτικό μήνυμα "SERVER TERMINATED BEFORE EXECUTION" στους αντίστοιχους job commanders που έχουν τα jobs τους ακόμα μέσα στην ουρά.

    3. Αδειάζει την ουρά `bufferQueue` του server.

    4. Κάνει ένα `pthread_cond_broadcast` στα condition variables που ενδεχομένως έχουν κοιμηθεί controllers, workers (`full`, `empty_or_max_concur` αντίστοιχα) για να μπορέσουν να τερματίσουν οι τελευταίοι, βλέποντας ότι το `exit_initiated` έχει τεθεί σε true.

    5. Στέλνει το μήνυμα "SERVER TERMINATED" στον jobCommander που πυροδότησε το exit και ο controller τερματίζει αφήνοντας πρώτα το `mtx`.

    Σε αυτή την φάση οποιοδήποτε άλλο thread (controller ή worker) πάρει το `mtx`, σε κάποιο από τα checks του `exit_initiated` θα οδηγηθεί στον τερματισμό του. Το main thread θα σταματήσει την loop του που ακούει συνεχώς για συνδέσεις και θα περιμένει τους workers (με το τρόπο που είπαμε παραπάνω) και τους controllers, με το εξής σχήμα συγχρονισμού:

    - Το main thread και όλοι οι controllers έχουν πρόσβαση στην κοινή global μεταβλητή `active_controllers` που προστατεύεται από το `controllers_mtx`. Κάθε φορά που ένας controller ξεκίνησει να εργάζεται, αυξάνεται ο counter `active_controllers` κατά 1 και κάθε φορά που σταματάει (είτε επειδή ολοκλήρωσε επιτυχώς το έργο του ή επειδή όφειλε να τερματίσει τον εαυτό του λόγω `exit_iniated == true`) μειώνει τον counter αυτόν κατά 1 και στην περίπτωση που αυτός ο counter μηδενιστεί, κάνει ένα `pthread_cond_signal` στο condition variable `last_controller_done`. Το main thread κοιμάται σε αυτό το condition variable (`last_controller_done`) όσο υπάρχουν ακόμη `active_controllers` (> 0). 

    - Με αυτό το σχήμα, επιτυγχάνουμε το main thread να περιμένει όλους τους controllers να τρέξουν τον κώδικα τους μέχρι (αλλά όχι και) το `pthread_exit` τους. Όπως ειπώθηκε και παραπάνω, ενδέχεται κάποιος ή κάποιοι controllers να μην προλάβουν να τρέξουν και την εντολή `pthread_exit` πριν τον τερματισμό του main thread, με αποτέλεσμα να υπάρξει κάποιο μικρό leak, παρόλ' αυτά δεν το λαμβάνουμε υπόψη καθώς θεωρείται 'αθώο', μιας και έχουν προλάβει (εγγυημένα) και έχουν εκτελέσει όλες τις απαραίτητες ενέργειες πιο πριν. Θυμίζουμε ότι, προφανώς, τα controller threads γίνονται detached αμέσως μετά την δημιουργία τους μέσω της `pthread_detach`. 

## Παραδοχές, διευκρινίσεις και δυνατότητες

- Παραδοχές
    - Για το connect στους jobCommanders χρησιμοποιείται η obsolete `gethostbyname` μιας και οι διδάσκοντες την επιτρέπουν και η δυνατότητα της εργασίας να λειτουργεί και σε άλλα versions του IP πέρα του IPv4 μάλλον ξεφεύγει από τα ζητούμενα της.

- Διευκρινίσεις
    - Μετά τον τερματισμό του server χρειάζεται λίγη ώρα για να εκκινήσει πάλι **στο ίδιο port** (`bind address already in use`).

    - Άμα σε κάποιο script δοκιμαστούν και άλλοι jobCommanders **αμέσως μετά** από κάποιον jobCommander τύπου exit, τότε αυτοί μπορεί να φάνε error στην connect (`connection refused`) ή, ενδέχεται να φάνε error πιο μετά στην read (`connection reset by peer`).

    - Όλα τα tests που υπάρχουν στον φάκελο `/tests` είναι αυτά της 1ης εργασίας του μαθήματος, τροποποιημένα έτσι ώστε να παίζουν στην παρούσα εργασία. Ο αναγνώστης θα πρέπει να δώσει μεγάλη προσοχή σε ενδεχόμενα race conditions. Σε μια προσπάθεια ανάδειξης της ορθότητας της εργασίας, έχουν τοποθετηθεί `sleep` σε αυτά τα test αρχεία, τα οποία ο αναγνώστης μπορεί εννοείται να αφαιρέσει/τροποποιήσει και ο server να εξακολουθεί να λειτουργεί σωστά. Σε μια τέτοια περίπτωση, όμως, θα χρειαστεί πολύ προσπάθεια για την κατανόηση και τον έλεγχο ορθότητας του output από τον χρήστη, λόγω των race conditions. Τέλος το `progDelay.c` είναι ακριβώς αυτό το αρχείο που μας δόθηκε στην πρώτη εργασία και το αντίστοιχο εκτέλεσιμο είναι απλά το εκτελέσιμο αυτού του αρχείου που έχει δημιουργηθεί μέσω `gcc -o` στα μηχανήματα της σχολής (ίδια αρχιτεκτονική με αυτή του εξεταστή). Εννοείται ο αναγνώστης μπορεί να διαγράψει αυτό το εκτελέσιμο και να το επαναμεταγλωττίσει, ειδικά σε περίπτωση που τρέχει την εργασία σε διαφορετική αρχιτεκτονική.

- Δυνατότητες 
    - Το main thread χρησιμοποιεί την `accept4()` (βλ. `handle_accept` στο `jobExecutorServer`) που είναι GNU extension, για να καλείται κάθε φορά με το flag `SOCK_CLOEXEC` και έτσι να μην είναι opened και ορατοί οι file descriptors των clients του server στα children processes που γίνονται spawned από τους workers.

    - Κάθε worker thread, μετά την ανάγνωση και την αποστολή του αρχείου `/output/[child_pid].output` στον αντίστοιχο client, σβήνει το αρχείο για να μην συσσωρευτούν πολλά τέτοια αρχεία στο σύστημα.

