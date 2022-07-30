/***
BSD 2-Clause License

Copyright (c) 2018, Adrián
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/


//
// Created by Adrián on 19/7/22.
//

#ifndef RING_LTJ_ALGORITHM_V2_HPP
#define RING_LTJ_ALGORITHM_V2_HPP


#include <triple_pattern.hpp>
#include <ring.hpp>
#include <ltj_iterator.hpp>
#include <gao.hpp>

namespace ring {

    template<class ring_t = ring<>, class var_t = uint8_t, class cons_t = uint64_t>
    class ltj_algorithm_v2 {

    public:
        typedef uint64_t value_type;
        typedef uint64_t size_type;
        typedef var_t var_type;
        typedef ring_t ring_type;
        typedef cons_t const_type;
        typedef ltj_iterator<ring_type, var_type, const_type> ltj_iter_type;
        typedef std::unordered_map<var_type, std::vector<ltj_iter_type*>> var_to_iterators_type;
        //TODO: darlle unha volta a esto
        typedef std::vector<std::pair<var_type, value_type>> tuple_type;
        typedef std::chrono::high_resolution_clock::time_point time_point_type;

    private:
        const std::vector<triple_pattern>* m_ptr_triple_patterns;
        std::vector<var_type> m_gao;
        ring_type* m_ptr_ring;
        std::vector<ltj_iter_type> m_iterators;
        var_to_iterators_type m_var_to_iterators;
        bool m_is_empty = false;


        void copy(const ltj_algorithm_v2 &o) {
            m_ptr_triple_patterns = o.m_ptr_triple_patterns;
            m_gao = o.m_gao;
            m_ptr_ring = o.m_ptr_ring;
            m_iterators = o.m_iterators;
            m_var_to_iterators = o.m_var_to_iterators;
            m_is_empty = o.m_is_empty;
        }


        //TODO: repasar o que pasa aqui
        inline void add_var_to_iterator(const var_type var, ltj_iter_type* ptr_iterator){
            auto it =  m_var_to_iterators.find(var);
            if(it != m_var_to_iterators.end()){
                it->second.push_back(ptr_iterator);
            }else{
                std::vector<ltj_iter_type*> vec = {ptr_iterator};
                m_var_to_iterators.insert({var, vec});
            }
        }

    public:


        ltj_algorithm_v2() = default;

        ltj_algorithm_v2(const std::vector<triple_pattern>* triple_patterns,
                      const std::vector<var_type>* gao,
                      ring_type* ring){

            m_ptr_triple_patterns = triple_patterns;
            m_gao = gao;
            m_ptr_ring = ring;

            size_type i = 0;
            m_iterators.resize(m_ptr_triple_patterns->size());
            for(const auto& triple : *m_ptr_triple_patterns){
                //Bulding iterators
                m_iterators[i] = ltj_iter_type(&triple, m_ptr_ring);
                if(m_iterators[i].is_empty){
                    m_is_empty = true;
                    return;
                }

                //For each variable we add the pointers to its iterators
                if(triple.o_is_variable()){
                    add_var_to_iterator(triple.term_o.value, &(m_iterators[i]));
                }
                if(triple.p_is_variable()){
                    add_var_to_iterator(triple.term_p.value, &(m_iterators[i]));
                }
                if(triple.s_is_variable()){
                    add_var_to_iterator(triple.term_s.value, &(m_iterators[i]));
                }
                ++i;
            }
            //TODO: escoger aqui el GAO

        }

        //! Copy constructor
        ltj_algorithm_v2(const ltj_algorithm_v2 &o) {
            copy(o);
        }

        //! Move constructor
        ltj_algorithm_v2(ltj_algorithm_v2 &&o) {
            *this = std::move(o);
        }

        //! Copy Operator=
        ltj_algorithm_v2 &operator=(const ltj_algorithm_v2 &o) {
            if (this != &o) {
                copy(o);
            }
            return *this;
        }

        //! Move Operator=
        ltj_algorithm_v2 &operator=(ltj_algorithm_v2 &&o) {
            if (this != &o) {
                m_ptr_triple_patterns = std::move(o.m_ptr_triple_patterns);
                m_gao = std::move(o.m_gao);
                m_ptr_ring = std::move(o.m_ptr_ring);
                m_iterators = std::move(o.m_iterators);
                m_var_to_iterators = std::move(o.m_var_to_iterators);
                m_is_empty = o.m_is_empty;
            }
            return *this;
        }

        void swap(ltj_algorithm_v2 &o) {
            std::swap(m_ptr_triple_patterns, o.m_ptr_triple_patterns);
            std::swap(m_gao, o.m_gao);
            std::swap(m_ptr_ring, o.m_ptr_ring);
            std::swap(m_iterators, o.m_iterators);
            std::swap(m_var_to_iterators, o.m_var_to_iterators);
            std::swap(m_is_empty, o.m_is_empty);
        }

        /**
         *
         * @param res               Results
         * @param limit_results     Limit of results
         * @param timeout_seconds   Timeout in seconds
         */
        void join(std::vector<tuple_type> &res,
                  const size_type limit_results = 0, const size_type timeout_seconds = 0){
            if(m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_gao.size());
            search(0, t, res, start, limit_results, timeout_seconds);
        };

        /**
        *
        * @param res               Results
        * @param limit_results     Limit of results
        * @param timeout_seconds   Timeout in seconds
        */
        void join_opt(std::vector<tuple_type> &res,
                  const size_type limit_results = 0, const size_type timeout_seconds = 0){
            if(m_is_empty) return;
            time_point_type start = std::chrono::high_resolution_clock::now();
            tuple_type t(m_gao.size());
            search_opt(0, t, res, start, limit_results, timeout_seconds);
        };

        /**
         *
         * @param j                 Index of the variable
         * @param tuple             Tuple of the current search
         * @param res               Results
         * @param start             Initial time to check timeout
         * @param limit_results     Limit of results
         * @param timeout_seconds   Timeout in seconds
         */
        void search(const size_type j, tuple_type &tuple, std::vector<tuple_type> &res,
                        const time_point_type start,
                        const size_type limit_results = 0, const size_type timeout_seconds = 0){

            //(Optional) Check timeout
            if(timeout_seconds > 0){
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop-start).count();
                if(sec > timeout_seconds) return;
            }

            //(Optional) Check limit
            if(limit_results > 0 && res.size() == limit_results) return;

            if(j == m_gao.size()){
                //Report results
                res.emplace_back(tuple);
            }else{
                var_type x_j = m_gao[j];
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                value_type c = seek(x_j);
                while(c != 0){ //If empty c=0
                    //1. Adding result to tuple
                    tuple[j] = {x_j, c};
                    //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                    for(ltj_iter_type* iter : itrs){
                        iter->down(x_j, c);
                    }
                    //4. Search with the next variable x_{j+1}
                    search(j+1, tuple, res, start, limit_results, timeout_seconds);
                    //3. Going up in the tries by removing x_j = c
                    for(ltj_iter_type* iter : itrs){
                        iter->up(x_j);
                    }
                    //5. Next constant for x_j
                    c = seek(x_j, c+1);
                }
            }
        };

        /**
         *
         * @param j                 Index of the variable
         * @param tuple             Tuple of the current search
         * @param res               Results
         * @param start             Initial time to check timeout
         * @param limit_results     Limit of results
         * @param timeout_seconds   Timeout in seconds
         */
        bool search_opt(const size_type j, tuple_type &tuple, std::vector<tuple_type> &res,
                    const time_point_type start,
                    const size_type limit_results = 0, const size_type timeout_seconds = 0){

            //(Optional) Check timeout
            if(timeout_seconds > 0){
                time_point_type stop = std::chrono::high_resolution_clock::now();
                auto sec = std::chrono::duration_cast<std::chrono::seconds>(stop-start).count();
                if(sec > timeout_seconds) return false;
            }

            //(Optional) Check limit
            if(limit_results > 0 && res.size() == limit_results) return false;

            if(j == m_gao.size()){
                //Report results
                res.emplace_back(tuple);
            }else{
                var_type x_j = m_gao.[j];
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                bool ok;
                if(itrs.size() == 1) {//Lonely variables
                    auto results = itrs[0]->seek_all(x_j);
                    //std::cout << "Results: " << results.size() << std::endl;
                    for (const auto &c : results) {
                        //1. Adding result to tuple
                        tuple[j] = {x_j, c};
                        //2. Going down in the trie by setting x_j = c (\mu(t_i) in paper)
                        itrs[0]->down(x_j, c);
                        //2. Search with the next variable x_{j+1}
                        ok = search_opt(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the trie by removing x_j = c
                        itrs[0]->up(x_j);
                    }
                }else {
                    value_type c = seek(x_j);
                    //std::cout << "Seek (init): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    while (c != 0) { //If empty c=0
                        //1. Adding result to tuple
                        tuple[j] = {x_j, c};
                        //2. Going down in the tries by setting x_j = c (\mu(t_i) in paper)
                        for (ltj_iter_type* iter : itrs) {
                            iter->down(x_j, c);
                        }
                        //3. Search with the next variable x_{j+1}
                        ok = search_opt(j + 1, tuple, res, start, limit_results, timeout_seconds);
                        if(!ok) return false;
                        //4. Going up in the tries by removing x_j = c
                        for (ltj_iter_type *iter : itrs) {
                            iter->up(x_j);
                        }
                        //5. Next constant for x_j
                        c = seek(x_j, c + 1);
                        //std::cout << "Seek (bucle): (" << (uint64_t) x_j << ": " << c << ")" <<std::endl;
                    }
                }
            }
            return true;
        };


        /**
         *
         * @param x_j   Variable
         * @param c     Constant. If it is unknown the value is -1
         * @return      The next constant that matches the intersection between the triples of x_j.
         *              If the intersection is empty, it returns 0.
         */
        value_type seek_base(const var_type x_j, value_type c=-1){
            while (true){
                value_type c_i, c_min = UINT64_MAX, c_max = 0;
                //Compute leap for each triple that contains x_j
                std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
                for(ltj_iter_type* iter : itrs){
                    if(c == -1){
                        c_i = iter->leap(x_j);
                    }else{
                        c_i = iter->leap(x_j, c);
                    }
                    if(c_i == 0) {
                        return 0; //Empty intersection
                    }
                    if(c_i > c_max) c_max = c_i;
                    if(c_i < c_min) c_min = c_i;
                }
                if(c_min == c_max) return c_min;
                c = c_max;
            }
        }


        value_type seek(const var_type x_j, value_type c=-1){
            value_type c_i, c_min = UINT64_MAX, c_max = 0;
            std::vector<ltj_iter_type*>& itrs = m_var_to_iterators[x_j];
            while (true){
                //Compute leap for each triple that contains x_j
                for(ltj_iter_type* iter : itrs){
                    if(c == -1){
                        c_i = iter->leap(x_j);
                    }else{
                        c_i = iter->leap(x_j, c);
                    }
                    if(c_i == 0) {
                        return 0; //Empty intersection
                    }
                    if(c_i > c_max) c_max = c_i;
                    if(c_i < c_min) c_min = c_i;
                    c = c_max;
                }
                if(c_min == c_max) return c_min;
                c_min = UINT64_MAX; c_max = 0;
            }
        }

    };
}

#endif //RING_LTJ_ALGORITHM_V2_HPP