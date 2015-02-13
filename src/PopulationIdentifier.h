#ifndef POPULATIONIDENTIFIER_H_
#define POPULATIONIDENTIFIER_H_
#include <string>
#include <unordered_map>
#include "fVcf.h"
#include "MathVector.h"
#include "MathGenMin.h"
class GenotypeMatrix{
public:
	fVcf tvcf;
	//int nInds;
	std::vector<std::string> chroms;
	std::vector<int> positions;
	std::vector<double> alleleFrequencies;
	std::vector<char> refBases;
	std::vector<char> altBases;

	std::vector<std::string> indids;

	std::vector<std::vector<float> > genotypes;
	//uint32_t numIndividual;
	//uint32_t numMarker;
	int bytesPerMarker;
	GenotypeMatrix();
	GenotypeMatrix(const char* vcfFile, bool siteOnly, bool findBest, double minAF, double minCallRate);
	int addMarker(const char* chrom, int position, char refBase, char altBase, double alleleFreq,int numIndividuals);
	void setGenotype(float genotype, int indIndex, int markerIndex);
	float getGenotype(int indIndex, int markerIndex);
	//double computeAlleleFrequency(int markerIndex);
	void printVCF(std::string path);
	~GenotypeMatrix();
};



class PopArgs {
public:
	std::string sSubsetInds;
	std::string sVcfFile;
	std::string sMpuFile;
	std::string sOutFile;
	std::string sSMID;
	std::string sScanPair;

	bool bSelfOnly;
	bool bSiteOnly;
	bool bFindBest;
	bool bIgnoreSex;

	//bool bIgnoreOverlapPair;
	bool bPrecise;
	bool bVerbose;
	bool bSilent;

	double genoError;
	double minAF;
	double minCallRate;
	double contamThres;

	double pRefRef;
	double pRefHet;
	double pRefAlt;

	bool bFreeNone;
	bool bFreeMixOnly;
	bool bFreeRefBiasOnly;
	bool bFreeFull;

	bool bChipNone;
	bool bChipMixOnly;
	bool bChipRefBiasOnly;
	bool bChipFull;

	int minMapQ;
	int maxDepth;
	int minQ;
	int maxQ;

	double grid;

	PopArgs() {
		bSelfOnly = false;
		bSiteOnly = false;
		bFindBest = false;
		bIgnoreSex = false;

		bPrecise = false;

		genoError = 0.001;
		minAF = 0.01;
		minCallRate = 0.50;
		contamThres = 0.02;

		pRefRef = 1;
		pRefHet = 0.5;
		pRefAlt = 0;

		bFreeNone = false;
		bFreeMixOnly = true;
		bFreeRefBiasOnly = false;
		bFreeFull = false;

		bChipNone = false;
		bChipMixOnly = true;
		bChipRefBiasOnly = false;
		bChipFull = false;

		bVerbose = false;

		minMapQ = 10;
		maxDepth = 20;
		minQ = 13;
		maxQ = 40;

		grid = 0.05;
	}
};

class PopulationIdentifier
{
public:

#define PCtype double
#define PHRED(x)	pow(10.0,x/-10.0)
	class fullLLKFunc : public VectorFunc {
	public:

		double llk0;
		double llk1;
		PopulationIdentifier* ptr;
		double PC1, PC2;
		fullLLKFunc();
		~fullLLKFunc();
		inline static double invLogit(double & x){ double e = exp(x); return e / (1. + e); };
		inline double computeMixLLKs(double tPC1, double tPC2)
		{
			double min_af(0.5 / ptr->NumIndividual), max_af((ptr->NumIndividual - 0.5) / ptr->NumIndividual);
			double sumLLK(0), GF0(0), GF1(0), GF2(0);
			std::string chr;
			uint32_t pos;
			for (size_t i = 0; i != ptr->NumMarker; ++i)
			{
				//std::cerr << "Number " << i << "th marker out of " << ptr->NumMarker << " markers and " << ptr->NumIndividual << " individuals"<<std::endl;
				//std::cerr << "AF:" << ptr->AFs[i] << "\tUD:" << ptr->UD[i][0] << "\t" << ptr->UD[i][1] << "\tmeans:" << ptr->means[i] << std::endl;
				chr = ptr->PosVec[i].first;
				pos = ptr->PosVec[i].second;
				ptr->AFs[i] = ((ptr->UD[i][0] * tPC1 + ptr->UD[i][1] * tPC2) + ptr->means[i]) / 2.0;
				if (ptr->AFs[i] < min_af) ptr->AFs[i] = min_af;
				if (ptr->AFs[i] > max_af) ptr->AFs[i] = max_af;
				GF0 = (1 - ptr->AFs[i])*(1 - ptr->AFs[i]);
				GF1 = 2 * (ptr->AFs[i])*(1 - ptr->AFs[i]);
				GF2 = (ptr->AFs[i])*(ptr->AFs[i]);
				sumLLK += log(PHRED(ptr->GL[ptr->MarkerIndex[chr][pos]][0]) * GF0 + PHRED(ptr->GL[ptr->MarkerIndex[chr][pos]][1]) * GF1 + PHRED(ptr->GL[ptr->MarkerIndex[chr][pos]][2]) * GF2);
				//std::cerr << "GL:" << ptr->GL[i][0] << "\t" << ptr->GL[i][1] << "\t" << ptr->GL[i][2] << "\t"<<ptr->GL.size()<<std::endl;
			}
			//std::cerr << "sumLLK:" << sumLLK << std::endl;
			return sumLLK;
		}
		fullLLKFunc(PopulationIdentifier* inPtr){
			ptr = inPtr;
			llk1 = llk0 = (0 - computeMixLLKs(static_cast<double>(ptr->PC[0][0]), static_cast<double>(ptr->PC[0][1])));
		}

		virtual double Evaluate(Vector& v) {
			if (v.Length() != 2)
				error("fullMixLLKFunc(): Input vector must be length of 3");

			double tmpPC1 = v[0];//invLogit(v[0]);
			double tmpPC2 = v[1]; //invLogit(v[1]);

			double smLLK = 0 - computeMixLLKs(tmpPC1, tmpPC2);

			if (smLLK < llk1) {
				llk1 = smLLK;
				PC1 = tmpPC1;
				PC2 = tmpPC2;
			}

			return smLLK;
		}
	};


	PopArgs* pArgs;
	GenotypeMatrix GenoMatrix;
	uint32_t NumMarker;
	uint32_t NumIndividual;
	fullLLKFunc fn;

	std::unordered_map<std::string, std::unordered_map<uint32_t, uint32_t> > MarkerIndex;
	//std::unordered_map<std::string, uint32_t> IndividualIndex;

	std::vector<std::vector<PCtype> > UD;
	std::vector<std::vector<PCtype> > PC;
	std::vector<std::vector<double> > GL;
	std::vector<PCtype> means;
	std::vector<double> AFs;

	std::unordered_map<std::string, std::unordered_map<int, int> > ChooseBed;
	std::vector<std::pair<std::string, int> > PosVec;

	PopulationIdentifier();
	/*Initialize from VCF*/
	/*This assumes the markers are from new VCF files which is different from the index vcf file*/
	PopulationIdentifier(const std::string& VCF,PopArgs* p, const std::string & GLpath);
	int ImputeMissing();
	int RunSVD();
	/*Initialize from existed UD*/
	/*This assumes the markers are the same as the selected vcf*/
	PopulationIdentifier(const std::string& UDpath, const std::string &PCpath, const std::string &Mean, const std::string & GLpath, const std::string &Bed);
	int ReadMatrixUD(const std::string &path);
	int ReadMatrixPC(const std::string &path);
	/*Intersect marker sites*/
	int ReadMatrixGL(const std::string& path);
	int ReadChooseBed(const std::string &path);
	int ReadMean(const std::string &path);
	int CheckMarkerSetConsistency();
	int FormatMarkerIntersection();
	/*Optimize*/
	int OptimizeLLK();
	int RunMapping();
	int PrintVcf(const std::string & path);
	~PopulationIdentifier();
};


#endif
