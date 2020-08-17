#ifndef CLASSIFIER_HPP
#define CLASSIFIER_HPP

#include <iostream>
#include <cmath>

#include "data.hpp"
#include "treegenerator.hpp"

class CClassifier
{
private:
    int m_nb_elmt_per_gen=100;
    int m_nb_gen=1;
    std::vector<CData*> m_ref_data;
    std::vector<std::map<Tree*,double> > m_generations;

public:
    CClassifier(){}
    ~CClassifier();
    CClassifier(const CClassifier&) = delete;

   // const Error& get_generation(int) const;

    void s_data(CData* data){m_ref_data.push_back(data);}
    void s_nb_elm_per_gen(int nb_elm){m_nb_elmt_per_gen = nb_elm;}
    void s_nb_gen(int nb_gen){m_nb_gen = nb_gen;}

    void compute_new_generation(CTreeGenerator*);
    void print_ref_data() const;
    void print_generation(int i);
    Tree* get_tree_for_specific_generation(int tr_id, int gen_id);
};

#endif // CLASSIFIER_HPP
