// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_PDELAB_BACKEND_DENSE_VECTOR_HH
#define DUNE_PDELAB_BACKEND_DENSE_VECTOR_HH

#include <algorithm>
#include <functional>
#include <numeric>

#include <dune/common/fvector.hh>
#include <dune/common/shared_ptr.hh>
#include <dune/istl/bvector.hh>

#include <dune/pdelab/backend/tags.hh>
#include <dune/pdelab/gridfunctionspace/gridfunctionspace.hh>
#include <dune/pdelab/gridfunctionspace/lfsindexcache.hh>
#include <dune/pdelab/gridfunctionspace/localvector.hh>
#include <dune/pdelab/backend/tags.hh>
#include <dune/pdelab/backend/backendselector.hh>
#include <dune/pdelab/backend/dense/descriptors.hh>

namespace Dune {
  namespace PDELab {

    namespace dense {

      namespace {

        // For some reason std::bind cannot directly deduce the correct version
        // of Dune::fvmeta::abs2, so we package it into a functor to help it along.
        template<typename K>
        struct abs2
        {
          typename FieldTraits<K>::real_type operator()(const K& k) const
          {
            return Dune::fvmeta::abs2(k);
          }
        };

      }

      template<typename GFS, typename C>
      class VectorContainer
      {

      public:
        typedef C Container;
        typedef typename Container::value_type ElementType;
        typedef ElementType E;
        typedef GFS GridFunctionSpace;
        typedef typename Container::size_type size_type;

        typedef typename GFS::Ordering::Traits::ContainerIndex ContainerIndex;

        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;

        template<typename LFSCache>
        struct LocalView
        {

          typedef E ElementType;
          typedef typename LFSCache::DOFIndex DOFIndex;
          typedef typename LFSCache::ContainerIndex ContainerIndex;

          LocalView()
            : _container(nullptr)
            , _lfs_cache(nullptr)
          {}

          LocalView(VectorContainer& container)
            : _container(&container)
            , _lfs_cache(nullptr)
          {}

          void attach(VectorContainer& container)
          {
            _container = &container;
          }

          void detach()
          {
            _container = nullptr;
          }

          void bind(const LFSCache& lfs_cache)
          {
            _lfs_cache = &lfs_cache;
          }

          void unbind()
          {
          }

          size_type size() const
          {
            return _lfs_cache->size();
          }

          template<typename LC>
          void read(LC& local_container) const
          {
            for (size_type i = 0; i < size(); ++i)
              {
                accessBaseContainer(local_container)[i] = (*_container)[_lfs_cache->containerIndex(i)];
              }
          }

          template<typename LC>
          void write(const LC& local_container)
          {
            for (size_type i = 0; i < size(); ++i)
              {
                (*_container)[_lfs_cache->containerIndex(i)] = accessBaseContainer(local_container)[i];
              }
          }

          template<typename LC>
          void add(const LC& local_container)
          {
            for (size_type i = 0; i < size(); ++i)
              {
                (*_container)[_lfs_cache->containerIndex(i)] += accessBaseContainer(local_container)[i];
              }
          }

          template<typename ChildLFS, typename LC>
          void read(const ChildLFS& child_lfs, LC& local_container) const
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                accessBaseContainer(local_container)[local_index] = (*_container)[_lfs_cache->containerIndex(local_index)];
              }
          }

          template<typename ChildLFS, typename LC>
          void write(const ChildLFS& child_lfs, const LC& local_container)
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                (*_container)[_lfs_cache->containerIndex(local_index)] = accessBaseContainer(local_container)[local_index];
              }
          }

          template<typename ChildLFS, typename LC>
          void add(const ChildLFS& child_lfs, const LC& local_container)
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                (*_container)[_lfs_cache->containerIndex(local_index)] += accessBaseContainer(local_container)[local_index];
              }
          }


          template<typename ChildLFS, typename LC>
          void read_sub_container(const ChildLFS& child_lfs, LC& local_container) const
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                accessBaseContainer(local_container)[i] = (*_container)[_lfs_cache->containerIndex(local_index)];
              }
          }

          template<typename ChildLFS, typename LC>
          void write_sub_container(const ChildLFS& child_lfs, const LC& local_container)
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                (*_container)[_lfs_cache->containerIndex(local_index)] = accessBaseContainer(local_container)[i];
              }
          }

          template<typename ChildLFS, typename LC>
          void add_sub_container(const ChildLFS& child_lfs, const LC& local_container)
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                (*_container)[_lfs_cache->containerIndex(local_index)] += accessBaseContainer(local_container)[i];
              }
          }

          void commit()
          {
          }


          ElementType& operator[](size_type i)
          {
            return (*_container)[_lfs_cache->containerIndex(i)];
          }

          const ElementType& operator[](size_type i) const
          {
            return (*_container)[_lfs_cache->containerIndex(i)];
          }

          ElementType& operator[](const DOFIndex& di)
          {
            return (*_container)[_lfs_cache->containerIndex(di)];
          }

          const ElementType& operator[](const DOFIndex& di) const
          {
            return (*_container)[_lfs_cache->containerIndex(di)];
          }

          ElementType& operator[](const ContainerIndex& ci)
          {
            return (*_container)[ci];
          }

          const ElementType& operator[](const ContainerIndex& ci) const
          {
            return (*_container)[ci];
          }

          VectorContainer& global_container()
          {
            return *_container;
          }

          const VectorContainer& global_container() const
          {
            return *_container;
          }

          const LFSCache& cache() const
          {
            return *_lfs_cache;
          }

        private:

          VectorContainer* _container;
          const LFSCache* _lfs_cache;

        };


        template<typename LFSCache>
        struct ConstLocalView
        {

          typedef E ElementType;
          typedef typename LFSCache::DOFIndex DOFIndex;
          typedef typename LFSCache::ContainerIndex ContainerIndex;

          ConstLocalView()
            : _container(nullptr)
            , _lfs_cache(nullptr)
          {}

          ConstLocalView(const VectorContainer& container)
            : _container(&container)
            , _lfs_cache(nullptr)
          {}

          void attach(const VectorContainer& container)
          {
            _container = &container;
          }

          void detach()
          {
            _container = nullptr;
          }

          void bind(const LFSCache& lfs_cache)
          {
            _lfs_cache = &lfs_cache;
          }

          void unbind()
          {
          }

          size_type size() const
          {
            return _lfs_cache->size();
          }

          template<typename LC>
          void read(LC& local_container) const
          {
            for (size_type i = 0; i < size(); ++i)
              {
                accessBaseContainer(local_container)[i] = (*_container)[_lfs_cache->containerIndex(i)];
              }
          }

          template<typename ChildLFS, typename LC>
          void read(const ChildLFS& child_lfs, LC& local_container) const
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                accessBaseContainer(local_container)[local_index] = (*_container)[_lfs_cache->containerIndex(local_index)];
              }
          }

          template<typename ChildLFS, typename LC>
          void read_sub_container(const ChildLFS& child_lfs, LC& local_container) const
          {
            for (size_type i = 0; i < child_lfs.size(); ++i)
              {
                const size_type local_index = child_lfs.localIndex(i);
                accessBaseContainer(local_container)[i] = (*_container)[_lfs_cache->containerIndex(local_index)];
              }
          }


          const ElementType& operator[](size_type i) const
          {
            return (*_container)[_lfs_cache->containerIndex(i)];
          }

          const ElementType& operator[](const DOFIndex& di) const
          {
            return (*_container)[_lfs_cache->containerIndex(di)];
          }

          const ElementType& operator[](const ContainerIndex& ci) const
          {
            return (*_container)[ci];
          }

          const VectorContainer& global_container() const
          {
            return *_container;
          }


        private:

          const VectorContainer* _container;
          const LFSCache* _lfs_cache;

        };


        VectorContainer(const VectorContainer& rhs)
          : _gfs(rhs._gfs)
          , _container(make_shared<Container>(*(rhs._container)))
        {}

        VectorContainer (const GFS& gfs, tags::attached_container = tags::attached_container())
          : _gfs(gfs)
          , _container(make_shared<Container>(gfs.ordering().blockCount()))
        {}

        //! Creates a VectorContainer without allocating storage.
        VectorContainer(const GFS& gfs, tags::unattached_container)
          : _gfs(gfs)
        {}

        VectorContainer (const GFS& gfs, const E& e)
          : _gfs(gfs)
          , _container(make_shared<Container>(gfs.ordering().blockCount(),e))
        {}

        void detach()
        {
          _container.reset();
        }

        void attach(shared_ptr<Container> container)
        {
          _container = container;
        }

        bool attached() const
        {
          return bool(_container);
        }

        const shared_ptr<Container>& storage() const
        {
          return _container;
        }

        size_type N() const
        {
          return _container->size();
        }

        VectorContainer& operator=(const VectorContainer& r)
        {
          if (this == &r)
            return *this;
          if (attached())
            {
              (*_container) = (*r._container);
            }
          else
            {
              _container = make_shared<Container>(*(r._container));
            }
          return *this;
        }

        VectorContainer& operator=(const E& e)
        {
          std::fill(_container->begin(),_container->end(),e);
          return *this;
        }

        VectorContainer& operator*=(const E& e)
        {
          std::transform(_container->begin(),_container->end(),_container->begin(),
                         std::bind(std::multiplies<E>(),e,std::placeholders::_1));
          return *this;
        }


        VectorContainer& operator+=(const E& e)
        {
          std::transform(_container->begin(),_container->end(),_container->begin(),
                         std::bind(std::plus<E>(),e,std::placeholders::_1));
          return *this;
        }

        VectorContainer& operator+=(const VectorContainer& y)
        {
          std::transform(_container->begin(),_container->end(),y._container->begin(),
                         _container->begin(),std::plus<E>());
          return *this;
        }

        VectorContainer& operator-= (const VectorContainer& y)
        {
          std::transform(_container->begin(),_container->end(),y._container->begin(),
                         _container->begin(),std::minus<E>());
          return *this;
        }

        E& operator[](const ContainerIndex& ci)
        {
          return (*_container)[ci[0]];
        }

        const E& operator[](const ContainerIndex& ci) const
        {
          return (*_container)[ci[0]];
        }

        typename Dune::template FieldTraits<E>::real_type two_norm() const
        {
          using namespace std::placeholders;
          typedef typename Dune::template FieldTraits<E>::real_type Real;
          return std::sqrt(std::accumulate(_container->begin(),_container->end(),Real(0),std::bind(std::plus<Real>(),_1,std::bind(abs2<E>(),_2))));
        }

        typename Dune::template FieldTraits<E>::real_type one_norm() const
        {
          using namespace std::placeholders;
          typedef typename Dune::template FieldTraits<E>::real_type Real;
          return std::accumulate(_container->begin(),_container->end(),Real(0),std::bind(std::plus<Real>(),_1,std::bind(std::abs<E>,_2)));
        }

        typename Dune::template FieldTraits<E>::real_type infinity_norm() const
        {
          if (_container->size() == 0)
            return 0;
          using namespace std::placeholders;
          typedef typename Dune::template FieldTraits<E>::real_type Real;
          return *std::max_element(_container->begin(),_container->end(),std::bind(std::less<Real>(),std::bind(std::abs<E>,_1),std::bind(std::abs<E>,_2)));
        }

        E operator*(const VectorContainer& y) const
        {
          return std::inner_product(_container->begin(),_container->end(),y._container->begin(),E(0));
        }

        E dot(const VectorContainer& y) const
        {
          return std::inner_product(_container->begin(),_container->end(),y._container->begin(),E(0),std::plus<E>(),Dune::dot<E,E>);
        }

        VectorContainer& axpy(const E& a, const VectorContainer& y)
        {
          using namespace std::placeholders;
          std::transform(_container->begin(),_container->end(),y._container->begin(),
                         _container->begin(),std::bind(std::plus<E>(),_1,std::bind(std::multiplies<E>(),a,_2)));
          return *this;
        }

        // for debugging and AMG access
        Container& base ()
        {
          return *_container;
        }

        const Container& base () const
        {
          return *_container;
        }

        iterator begin()
        {
          return _container->begin();
        }

        const_iterator begin() const
        {
          return _container->begin();
        }

        iterator end()
        {
          return _container->end();
        }

        const_iterator end() const
        {
          return _container->end();
        }

        size_t flatsize() const
        {
          return _container->size();
        }

        const GFS& gridFunctionSpace() const
        {
          return _gfs;
        }

      private:
        const GFS& _gfs;
        shared_ptr<Container> _container;
      };


    }


#ifndef DOXYGEN

    template<typename GFS, typename E>
    struct DenseVectorSelectorHelper
    {

      using vector_type = typename GFS::Traits::Backend::template vector_type<E>;

      using Type = dense::VectorContainer<GFS,vector_type>;

    };

    template<template<typename> class Container, typename GFS, typename E>
    struct BackendVectorSelectorHelper<DenseVectorBackend<Container>, GFS, E>
      : public DenseVectorSelectorHelper<GFS,E>
    {};

#endif // DOXYGEN

  } // namespace PDELab
} // namespace Dune

#endif // DUNE_PDELAB_BACKEND_DENSE_VECTOR_HH
