typedef struct subasta *nSubasta;

void fatalError( char *procname, char *format, ... );

nSubasta nNuevaSubasta(int unidades);
int nOfrecer(nSubasta s, double precio, int timeout);
double nAdjudicar(nSubasta s, int *punidades);
void nDestruirSubasta(nSubasta s);

enum { FALSE= 0, TRUE= 1 };
