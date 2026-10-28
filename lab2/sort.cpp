#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <pthread.h>
#include <semaphore.h>
#include <fstream>

static bool read_array_from_file(const std::string& path, std::vector<int>& a) {
            std::ifstream f(path);
            if (!f) {
                std::fprintf(stderr, "Cannot open file: %s\n", path.c_str());
                return false;
            }
            a.clear();
            int x;
            while (f >> x) {
                a.push_back(x);
            }
            if (!f.eof()) {
                   std::fprintf(stderr, "Malformed number in file: %s\n", path.c_str());
                   return false;
            }
            if (a.empty()) {
                   std::fprintf(stderr, "File is empty: %s\n", path.c_str());
                   return false;
            }
            return true;
}

static bool write_array_to_file(const std::string& path, const std::vector<int>& a) {
            std::ofstream f(path);
            if (!f) {
                std::fprintf(stderr, "Cannot open for writing: %s\n", path.c_str());
                return false;
            }
            for (int x : a) f << x << '\n';
            return f.good();
}

static double now_sec(){
        using namespace std::chrono;
        return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static void insertion_sort(int* a, int l, int r){
        for (int i = l+1; i < r; i++){
                int x = a[i];
                int j = i-1;
                while ((j >= l) && (a[j] > x)){
                        a[j+1] = a[j];
                        j--;
                }
                a[j+1] = x;
        }
}

static void partition3(int* a, int l, int r, int& lt, int& gt, int pivot){
        int i = l;
         int less = l, greater = r;
         while (i < greater){
                 if (a[i] < pivot){
                         std::swap(a[less], a[i]);
                         less++; i++;
                 } else if (a[i] > pivot){
                         greater--;
                         std::swap(a[greater], a[i]);
                 } else {
                         i++;
                 }
         }
         lt = less;
         gt = greater;
}

static int choose_pivot(const int *a, int l, int r) {
        int m = l + (r - l) / 2;
        int x = a[l], y = a[m], z = a[r - 1];
        if ((x <= y && y <= z) || (z <= y && y <= x)) return y;
        if ((y <= x && x <= z) || (z <= x && x <= y)) return x;
        return z;
}

static void seq_quicksort(int* a, int l, int r){
        while (r - l > 1){
                if (r - l < 32){
                        insertion_sort(a, l, r);
                        return;
                }
                int lt, gt;
                int pivot = choose_pivot(a, l, r);
                partition3(a, l, r, lt, gt, pivot);
                if (lt - l < r - gt){
                        seq_quicksort(a, l, lt);
                        l = gt;
                } else{
                        seq_quicksort(a, gt, r);
                        r = lt;
                }
        }
}

static bool is_sorted(const std::vector<int>& a){
        for (size_t i = 1; i < a.size(); i++){
                if (a[i-1] > a[i]) return false;
        }
        return true;
}

static const int kSeqThreshold = 4096;
static sem_t sem;

struct ThreadArg {
        int *a;
        int l;
        int r;
};

static void par_sort(int*a, int l, int r);

static void* worker(void* p){
        ThreadArg arg = *(ThreadArg*) p;
        free(p);
        par_sort(arg.a, arg.l, arg.r);
        return nullptr;
}

static void par_sort(int*a, int l, int r){
        if (r - l <= 1) return;
        if (r - l < kSeqThreshold){
                seq_quicksort(a, l, r);
                return;
        }
        int lt, gt;
        int pivot = choose_pivot(a, l, r);
        partition3(a, l, r, lt, gt, pivot);

        pthread_t tid;
        bool spawned = false;
        if (sem_trywait(&sem) == 0){
                ThreadArg* arg = (ThreadArg*)malloc(sizeof(ThreadArg));
                if (arg){
                        arg->a = a;
                        arg->l = gt;
                        arg->r = r;
                        if (pthread_create(&tid, nullptr, worker, arg) == 0){
                                spawned = true;
                        } else{
                                free(arg);
                                sem_post(&sem);
                        }
                } else {
                        sem_post(&sem);
                }
        }

        par_sort(a, l, lt);

        if(spawned){
                pthread_join(tid, nullptr);
                sem_post(&sem);
        } else{
                par_sort(a, gt, r);
        }
}

static void par_quicksort(int* a, int n, int threads){
        if (n <= 1) return;
        if (threads <= 1) {
                seq_quicksort(a, 0, n);
                return;
        }
        sem_init(&sem, 0, threads - 1);
        par_sort(a, 0, n);
        sem_destroy(&sem);
}


int main(int argc, char** argv) {
        std::string input_file;
        int threads = 1;
        for (int i = 1; i < argc; ++i) {
                std::string k = argv[i];
                if (k == "-f"){
                        if (i + 1 >= argc) { std::fprintf(stderr, "Missing value for -f\n"); return EXIT_FAILURE;}      
                        input_file = argv[++i];
                } else if (k == "-t"){
                        if (i + 1 >= argc) { std::fprintf(stderr, "Missing value for -t\n"); return EXIT_FAILURE; }     
                        threads = std::atoi(argv[++i]);
                } else {
                        std::fprintf(stderr, "Unknown argument: %s\n", k.c_str());
                        return EXIT_FAILURE;
                }
        }
        if (input_file.empty() || threads <= 0) return 1;
        std::vector<int>original;
        if (!read_array_from_file(input_file, original)) return EXIT_FAILURE;
        int n =(int)original.size();

        std::vector<int> work(original);

        double t0 = now_sec();
        seq_quicksort(work.data(), 0, n);
        double t1 = now_sec();
        double T_seq = t1 - t0;

        if (!is_sorted(work)){
                std::fprintf(stderr, "seq: not sorted\n");
                return EXIT_FAILURE;
        }

        work = original;
        t0 = now_sec();
        par_quicksort(work.data(), n, threads);
        t1 = now_sec();
        double T_par = t1 - t0;

        if (!is_sorted(work)){
                std::fprintf(stderr, "par: not sorted\n");
                return EXIT_FAILURE;
        }
        if(!write_array_to_file("result.txt", work)) {
                std::fprintf(stderr, "Cannot write to file\n");
                return EXIT_FAILURE;
        }
        double S = T_seq / T_par;
        double X = S / threads;


        std::printf("Количество элементов в массиве: %d\n Максимальное количество потоков: %d\n", n, threads);
        std::printf("Время работы последовательной сортировки: %.6fсек\n", T_seq);
        std::printf("Время работы параллельной сортировки: %.6fсек\n", T_par);
        std::printf("Ускорение S = %.6f\n", S);
        std::printf("Эффективность X = %.6f\n", X);
        return EXIT_SUCCESS;
}
