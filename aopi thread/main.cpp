#include <iostream>
#include <thread>
#include <EventSys.h>
#include <synchapi.h>
#include <cassert>
#include <windows.h>

using namespace std;

#define THREADS_NUMBER 2
#define TIMER 1000
struct point {
	double x;
	double y;
};

#pragma region WorkWithData

double readValue(istream& in) {
	double result;
	if (in >> result) {
		return result;
	}

	cout << "Wrong value" << endl;
	system("pause");
	exit(0);
}

point readPoint(istream& in) {
	point p;
	p.x = readValue(in);
	p.y = readValue(in);
	return p;
}

void readArray(istream& in, point** array, int *size) {
	assert(array);
	cout << "Enter value of points for interpolation" << endl;
	cout << ">  ";
	int N = int(readValue(in));
	point* values = new point[N];

	cout << "Enter coords of points for interpolation in the same way :" << endl;
	cout << "1 2" << endl;
	cout << "3 4" << endl;
	cout << "5 6" << endl;
	for (int i = 0; i < N; ++i) {
		cout << ">  ";
		values[i] = readPoint(in);
	}

	*array = values;
	if (size) {
		*size = N;
	}
}

void printArray(ostream& out, point *p, int N) {
	out << N << endl;
	for (int i = 0; i < N; ++i) {
		out << "(" << p[i].x << "," << p[i].y << ")" << endl;
	}
}


#pragma endregion

#pragma region SortingData


typedef bool(*comparator)(double, double);
bool less_compare(double a, double b) { return a < b; }

int parent(int i) { return (i - 1) / 2; }
int left(int i) { return 2 * i + 1; }
int right(int i) { return 2 * i + 2; }

void siftDown(point *p, int start, int end, comparator cmp) {
	int root = start;
	while (left(root) <= end) {
		int swapWith = root;

		int leftChild = left(root);
		if (cmp(p[swapWith].x, p[leftChild].x)) {
			swapWith = leftChild;
		}
		int rightChild = right(root);
		if (rightChild <= end && cmp(p[swapWith].x, p[rightChild].x)) {
			swapWith = rightChild;
		}

		if (swapWith == root) {
			return;
		}

		swap(p[swapWith], p[root]);
		root = swapWith;
	}
}

void heapify(point *p, int value, comparator cmp) {
	for (int i = parent(value - 1); i >= 0; --i) {
		siftDown(p, i, value - 1, cmp);
	}
}

void heapSort(point *p, int value, comparator cmp) {
	heapify(p, value, cmp);
	for (int i = value - 1; i > 0; --i) {
		swap(p[i], p[0]);
		siftDown(p, 0, i - 1, cmp);
	}
}

#pragma endregion

#pragma region InterpolatingData


double dividedDifferences(point *p, int i, int j) {
	if (i == j) { return p[i].y; }
	return (dividedDifferences(p, i + 1, j) - dividedDifferences(p, i, j - 1)) / (p[j].x - p[i].x);
}

double xVal(point *p, double x, int k) {
	double product = 1;
	for (int i = 0; i < k; ++i) {
		product *= (x - p[i].x);
	}
	return product;
}

double computeNewtonPoly(point *p, int N, double x) {
	double result = 0;
	for (int i = 0; i < N; ++i) {
		result += dividedDifferences(p, 0, i)*xVal(p, x, i);
	}
	return result;
}

#pragma endregion

DWORD SortThread(point *p, int N) {
	HANDLE H = CreateEventA(NULL, true, false, "MainThread");
	WaitForSingleObject(H, TIMER);
	heapSort(p, N, less_compare);
	SetEvent(OpenEvent(EVENT_ALL_ACCESS, true, "ME2"));
	ExitThread(0);
}

DWORD InterpolationThread(point *p, int N) {
	HANDLE E = CreateEventA(NULL, true, false, "ME2");
	WaitForSingleObject(E, TIMER);
	bool k = true;
	while (k) {
		double x = readValue(cin);
		if (x == 999.){
			k = false;
		}
		cout << computeNewtonPoly(p, N, x) << endl << endl;
	}
	SetEvent(OpenEvent(EVENT_ALL_ACCESS, true, "ME3"));
	ExitThread(0);
}

int main() {
	point *array = nullptr;
	int N;

	HANDLE e[THREADS_NUMBER];

	e[0] = CreateEventA(NULL, true, false, "Sort");
	e[1] = CreateEventA(NULL, true, false, "Interpolator");

	HANDLE H = CreateEventA(NULL, true, false, "MainThread");

	//подготовка данных
	readArray(cin, &array, &N);
	thread sort(SortThread, array, N);
	//сообщение об окончании подготовки
	SetEvent(H);
	//ожидание завершения потоков
	thread interpolation(InterpolationThread, array, N);
	WaitForMultipleObjects(2, e, true, INFINITE);

	for (int i = 0; i < THREADS_NUMBER; ++i) {
		ResetEvent(e[i]);
	}
	ResetEvent(H);

	for (int i = 0; i < THREADS_NUMBER; ++i) {
		CloseHandle(e[i]);
		//CloseHandle(hThreads[i]);
	}
	CloseHandle(H);
	return 0;
}
