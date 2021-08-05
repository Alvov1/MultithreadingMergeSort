#include <iostream>
#include <pthread.h>
#include <vector>
#include <iterator>
#include <fstream>
#include <algorithm>
#include <chrono>

unsigned threadAmount = 0;
std::vector<pthread_t*> Threads;

unsigned numbersAmount = 0;
std::vector<unsigned> Numbers;

unsigned numbersPerThread = 0;
unsigned remainNumbers = 0;


std::chrono::high_resolution_clock::time_point timeMark;
std::chrono::duration<double, std::milli> timeSpent;

class sortingData {
public:
    sortingData(const unsigned number, const unsigned size, unsigned border) :
            threadNumber(number),
            subarraySize(size),
            firstElem(border){
    };

    unsigned FirstElem() const{
        return this->firstElem;
    }
    unsigned Size() const{
        return this->subarraySize;
    }
    unsigned ThreadNumber() const{
        return this->threadNumber;
    }

private:
    unsigned threadNumber;
    unsigned subarraySize;
    unsigned firstElem;
};
std::vector<sortingData*> threadInfos;

/* -------------------------------------------------------------------------------------- */
template<typename It>
std::vector<typename It::value_type> merge(const It begin, const It mid, const It end)
{
    std::vector<typename It::value_type> v;
    It it_l{ begin }, it_r{ mid };
    const It it_mid{ mid }, it_end{ end };

    while (it_l != it_mid && it_r != it_end)
    {
        v.push_back((*it_l <= *it_r) ? *it_l++ : *it_r++);
    }

    v.insert(v.end(), it_l, it_mid);
    v.insert(v.end(), it_r, it_end);

    return std::move(v);
}
template<typename It>
void MergeSort(It begin, It end)
{
    auto size = std::distance(begin, end);

    /* --------------------- */
    /* На малых размерах массива, сортировка слиянием перестает быть эффективной.
     * Следующее решение позволяет почти в полтора раза уменьшить стек вызовов и избежать его переполнения:
     *
     * Тест с наибольшем количеством элементов содержит 10М чисел, что примерно эквивалентно 2^24.
     * При переходе с размером массива <= 256 к нерекурссивной сортировке, стек становится на 8 меньше.*/
    if (size < 257) {
        std::sort(begin, end);
        return;
    }
    /* --------------------- */

    auto mid = std::next(begin, size / 2);
    MergeSort(begin, mid);
    MergeSort(mid, end);

    auto &&v = merge(begin, mid, end);
    std::move(v.cbegin(), v.cend(), begin);
}
/* ---------------------------------------------------------------------------------- */
void* threadEntry(void* param){
    auto* data = (sortingData*) param;

    const unsigned threadNumber = data->ThreadNumber();
    const unsigned subarraySize = data->Size();

    auto subarrayStart = Numbers.begin();
    auto subarrayEnd = Numbers.begin();
    std::advance(subarrayStart, data->FirstElem());
    std::advance(subarrayEnd,subarraySize + data->FirstElem());

    if(subarrayEnd > subarrayStart)
        MergeSort(subarrayStart, subarrayEnd);
    return nullptr;
}
int threadsCreate(){
    Threads.reserve(threadAmount);

    for(auto threadNumber = 0; threadNumber < threadAmount; threadNumber++){
        auto* temp = new pthread_t;
        Threads.push_back(temp);

        if(pthread_create(temp, nullptr, threadEntry, (void*) (threadInfos[threadNumber])) != 0){
            std::cerr << "Error with thread creation" << std::endl;
            return -1;
        }
    }

    for(auto threadNumber = 0; threadNumber < threadAmount; threadNumber++){
        auto* temp = Threads[threadNumber];
        pthread_join(*temp, nullptr);
        delete temp;
    }

    Threads.clear();
    Threads.shrink_to_fit();

    MergeSort(Numbers.begin(), Numbers.end());

    return 0;
}
/* -------------------------------------------------------------------------------------- */
void prepareData(){
    threadInfos.reserve(threadAmount);

    unsigned subarrayStart = 0;
    for(unsigned threadNumber = 0; threadNumber < threadAmount; threadNumber++){

        const unsigned subarraySize = numbersPerThread + ((remainNumbers != 0 && threadNumber < remainNumbers) ? 1 : 0);
        auto temp = new sortingData(threadNumber, subarraySize, subarrayStart);
        threadInfos.push_back(temp);

        subarrayStart += subarraySize;
    }
}
/* ---------------------------------------------------------------------------------- */
int readArguments(const std::string& filename = "input.txt"){
    FILE* inputStream = fopen(filename.c_str(), "r");
    if(inputStream == nullptr){
        std::cerr << "Error with input file" << std::endl << "Terminating..." << std::endl;
        return -1;
    }

    fscanf(inputStream, "%u\n%u\n", &threadAmount, &numbersAmount);
    Numbers.reserve(numbersAmount);

    unsigned temp;
    for(auto i = 0; i < numbersAmount; i++){
        fscanf(inputStream, "%u ", &temp);
        Numbers.push_back(temp);
    }
    return 0;
}
int writeResults(const std::string& outputName = "output.txt", const std::string& timeName = "time.txt"){
    FILE* outputStream = fopen(outputName.c_str(), "w");
    if(outputStream == nullptr){
        std::cerr << "Errors with the output file" << std::endl << "Terminating..." << std::endl;
        return -1;
    }

    fprintf(outputStream, "%u\n%u\n", threadAmount, numbersAmount);
    for(auto i = 0; i < numbersAmount; i++)
        fprintf(outputStream, "%u ", Numbers[i]);

    std::ofstream timeFile(timeName);
    timeFile << timeSpent.count();
    timeFile.close();

    Numbers.clear();
    Numbers.shrink_to_fit();
    return 0;
}
/* ---------------------------------------------------------------------------------- */

/* Разобьем весь массив поровну между созданными потоками, отсортируем участки.
 * Слияние проведем в обычном потоке. */

int main(const int argc, const char** argv) {
    if(argc != 1){
        readArguments(std::string(argv[1]));
    } else readArguments();

    std::cout << "Threads number = " << threadAmount << std::endl;
    std::cout << "Numbers = " << numbersAmount << std::endl;

    numbersPerThread = numbersAmount / threadAmount;
    remainNumbers = numbersAmount % threadAmount;

    prepareData();

    if(threadAmount == 1 || threadAmount >= numbersAmount){
        timeMark = std::chrono::high_resolution_clock::now();
        MergeSort(Numbers.begin(), Numbers.end());
        timeSpent = (std::chrono::high_resolution_clock::now() - timeMark);
    } else {
        timeMark = std::chrono::high_resolution_clock::now();

        threadsCreate();

        timeSpent = (std::chrono::high_resolution_clock::now() - timeMark);
    }

    std::cout << "Time spent: " << timeSpent.count() << std::endl;

    writeResults();
    return 0;
}
