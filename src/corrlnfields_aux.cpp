// These are functions created specifically for the corrlnfields program.
#include "corrlnfields_aux.hpp"

/*** Multiply Lower-triangular matrix L to complex vector gaus0 and return gaus1 ***/
void CorrGauss(double **gaus1, gsl_matrix *L, double **gaus0) {
  long i, j;

  for(i=0; i<L->size1; i++) {
    gaus1[i][0]=0; // Re    
    gaus1[i][1]=0; // Im
    for(j=0; j<=i; j++) { // L matrix stored as vector in row-major order.
      gaus1[i][0] += L->data[i*L->size1+j] * gaus0[j][0];
      gaus1[i][1] += L->data[i*L->size1+j] * gaus0[j][1];
    }
  }
}


/*** Transforms a correlation function of lognormal variables lnXi into a corr. function of associated gaussian variables gXi ***/
int GetGaussCorr(double *gXi, double *lnXi, int XiLength, double mean1, double shift1, double mean2, double shift2) {
  int i, status=0;
  double arg, bad=-666.0;
  char message[100];
  
  for (i=0; i<XiLength; i++) {
    arg = 1.0 + lnXi[i]/(mean1+shift1)/(mean2+shift2);
    if (arg <= 0) {
      sprintf(message, "GetGaussCorr: lnXi[%d] leads to bad log argument, gXi[%d] set to %g.", i, i, bad);
      warning(message);
      status=EDOM;
      gXi[i] = bad;
    }
    else gXi[i] = log(arg);
  }
  return status;
}


/*** Transform Cov. matrix of lognormal variables into 
     the Cov. matrix of associated gaussian variables  ***/
int GetGaussCov(gsl_matrix *gCovar, gsl_matrix *lnCovar, double *means, double *shifts) {
  double arg;
  long i, j;
  char message[100]; int status=0; double bad=-666.0;
  
  for(i=0; i<lnCovar->size1; i++)
    for(j=0; j<lnCovar->size2; j++) {
      arg = lnCovar->data[i*(lnCovar->size1)+j]/(means[i]+shifts[i])/(means[j]+shifts[j]) + 1.0;
      // If arg<0, it is an error.
      if (arg<=0) {
	sprintf(message,"GetGaussCov: lnCovar[%ld][%ld] leads to bad log argument, gCovar[%ld][%ld] set to %g.",i,j,i,j,bad);
	warning(message);
	status=EDOM;
	gCovar->data[i*(gCovar->size1)+j] = bad;
      }
      // If arg>0, it is valid.
      else gCovar->data[i*(gCovar->size1)+j] = log(arg);  
    }
  return status;
}


/*** Compute a log-normal variable from a gaussian one ***/
double Gauss2LNvar(double gvar, double mean, double variance, double shift) {
  double expsigma2, expmu;
  
  expsigma2 = 1 + variance/pow(mean+shift,2);
  expmu = (mean+shift)/sqrt(expsigma2);

  return expmu*exp(gvar)-shift;
}


/*** Get a number that specify the l of the cov. matrix ***/
int getll(const std::string filename) {
  int i=0, num=0, fileL;
  
  fileL=filename.length();
  // Find a number:
  while (isdigit(filename.c_str()[i])==0) {i++; if(i>=fileL) error("getll: cannot find any number.");}
  // Read the number:
  while (isdigit(filename.c_str()[i])!=0) {num = num*10 + (filename.c_str()[i]-'0'); i++;}
  // Check if there are more numbers in filename:
  while (i<=fileL) {
    if (isdigit(filename.c_str()[i])!=0) error("getll: found more numbers than expected.");
    i++;
  }

  // Return number found:
  return num;
}


/*** Returns getll as string ***/
std::string getllstr(const std::string filename) {
  std::stringstream ss;
  ss << getll(filename);
  return ss.str();
}


/*** Function for writing the header of alm's file ***/
std::string SampleHeader(std::string fieldsfile) {
  std::stringstream ss;
  int **fz; 
  long nr, nc, i, j;

  fz = LoadTable<int>(fieldsfile, &nr, &nc, 1);
  if (nc!=2) error("SampleHeader: expect 2 columns in file "+fieldsfile);
 
  ss << "# l, m";
  for (i=1; i<=nr; i++) ss << ", f" << fz[i][1] << "z" << fz[i][2] << "Re, f" << fz[i][1] << "z" << fz[i][2] << "Im";
  
  return ss.str(); 
}


/*** Function for writing the header of alm's file ***/
std::string SampleHeader(int **fnz, int nr) {
  std::stringstream ss;
  int i;
  
  ss << "# l, m";
  for (i=0; i<nr; i++) ss << ", f" << fnz[i][0] << "z" << fnz[i][1] << "Re, f" << fnz[i][0] << "z" << fnz[i][1] << "Im";
  
  return ss.str(); 
}



/*** Get four numbers separated by characters that specify the fields and redshifts
     of the correlation function. ***/
void getcovid(const std::string filename, int *a1, int *a2, int *b1, int *b2) {
  int i=0, num, index, fileL;
  
  fileL=filename.length();
  // LOOP over the four indexes that indentifies the C(l):
  for(index=1; index<=4; index++) {
    num=0;
    // Find a number:
    while (isdigit(filename.c_str()[i])==0) {i++; if(i>=fileL && index!=4) error("getcovid: cannot find four numbers.");}
    // Read the number:
    while (isdigit(filename.c_str()[i])!=0) {num = num*10 + (filename.c_str()[i]-'0'); i++;}
    // Save it as an index:
    switch (index) {
    case 1: *a1 = num; break;
    case 2: *a2 = num; break;
    case 3: *b1 = num; break;
    case 4: *b2 = num; break;
    }
  }
  // Check if there are more numbers in filename:
  while (i<=fileL) {
    if (isdigit(filename.c_str()[i])!=0) error("getcovid: found more numbers than expected.");
    i++;
  }
}


/*** Find out number of columns and rows in file ***/
void CountEntries(std::string filename, long *nr, long *nc) {
  using std::ifstream;
  using std::string;
  using std::istringstream;
  using std::ostringstream;
  long nrows=0, ncols=0;
  ifstream file;
  istringstream inputline; ostringstream outputline;
  string word, phrase;
  
  // Open file
  file.open(filename.c_str());
  if (!file.is_open()) error("CountEntries: cannot open file.");
  
  // Count lines and columns:
  getline(file,phrase);
  outputline << phrase;
  inputline.str(outputline.str());
  while (inputline >> word) ncols++;
  while(!file.eof()) {getline(file,phrase); nrows++;}

  file.close();
  *nr=nrows+1;
  *nc=ncols;
}


/*** Assign a matrix column n to a variable 'a' identified by a1 and a2  ***/
void fz2n (int a1, int a2, int *n, int N1, int N2) {
  if (a2>N2 || a1>N1 || a1<1 || a2<1) warning("fz2n: unexpected input values.");
  *n = (a1-1)*N2+a2-1; 
}


/*** The inverse of ij2fzfz above ***/
void n2fz (int n, int *a1, int *a2, int N1, int N2) {
  if (n<0 || n>=N1*N2) warning("n2fz: unexpected input values.");
  *a2 = n%N2+1;
  *a1 = n/N2+1;
}


/*** Assign a matrix row i to a variable 'a' identified by a1 and a2 ***/
/*** Assign a matrix column j to a variable 'b' identified by b1 and b2  ***/
void fzfz2ij (int a1, int a2, int b1, int b2, int *i, int *j, int N1, int N2) {
  if (a2>N2 || b2>N2 || a1>N1 || b1>N1 || a1<1 || a2<1 || b1<1 || b2<1) warning("fzfz2ij: unexpected input values.");
  fz2n(a1, a2, i, N1, N2);
  fz2n(b1, b2, j, N1, N2);
}

/*** The inverse of ij2fzfz above ***/
void ij2fzfz (int i, int j, int *a1, int *a2, int *b1, int *b2, int N1, int N2) {
  if (i<0 || j<0 || i>=N1*N2 || j>=N1*N2) warning("ij2fzfz: unexpected input values.");
  n2fz(i, a1, a2, N1, N2);
  n2fz(j, b1, b2, N1, N2);
}


/*** Function for testing the assignments above ***/
void test_fzij (int N1, int N2) {
  int a1, a2, b1, b2, i, j, newa1, newa2, newb1, newb2;
  bool **IsSet;

  IsSet = matrix<bool>(0,N1*N2-1,0,N1*N2-1);
  for(i=0; i<N1*N2; i++) for(j=0; j<N1*N2; j++) IsSet[i][j]=0;
  
  for (a1=1; a1<=N1; a1++)
    for (a2=1; a2<=N2; a2++)
      for (b1=1; b1<=N1; b1++)
	for (b2=1; b2<=N2; b2++) {
	  fzfz2ij(a1, a2, b1, b2, &i, &j, N1, N2); 
	  if (IsSet[i][j]==1) error("test_fzij: tried to set [i,j] already set.");
	  IsSet[i][j]=1;
	  ij2fzfz(i, j, &newa1, &newa2, &newb1, &newb2, N1, N2);
	  if(newa1!=a1 || newa2!=a2 || newb1!=b1 || newb2!=b2) error("test_fzij: function ij2fzfz not the inverse of fzfz2ij."); 
	}
  for(i=0; i<N1*N2; i++) for(j=0; j<N1*N2; j++) if (IsSet[i][j]!=1) error("Matrix [i,j] not fully populated.");

  free_matrix(IsSet,0,N1*N2-1,0,N1*N2-1);
}



/*** Export function y(x) for the field combination [i,j] to file ***/
std::string PrintOut(std::string prefix, int i, int j, int N1, int N2, double *x, double *y, int length) {
  int a1, a2, b1, b2;
  char message[100];
  std::string filename;
  double *wrapper[2];
  std::ofstream outfile;

  wrapper[0] =  x;
  wrapper[1] =  y;

  n2fz(i, &a1, &a2, N1, N2); n2fz(j, &b1, &b2, N1, N2);
  sprintf(message, "%sf%dz%df%dz%d.dat", prefix.c_str(),a1,a2,b1,b2);
  filename.assign(message);

  outfile.open(message);
  if (!outfile.is_open()) error("PrintOut: cannot open file "+filename);
  PrintVecs(wrapper, length, 2, &outfile);
  outfile.close();

  return filename;
}
