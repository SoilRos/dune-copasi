#ifndef DUNE_COPASI_CONTEXT_GEOMETRY_TYPE_HH
#define DUNE_COPASI_CONTEXT_GEOMETRY_TYPE_HH

#include <dune/copasi/concepts/has_method.hh>
#include <dune/copasi/context/base.hh>

#include <dune/geometry/type.hh>

#include <type_traits>

namespace Dune::Copasi::Context {

template<class Ctx, class = std::void_t<>>
struct GeometryTypeCtx : public Ctx
{
  using GeometryType = Dune::GeometryType;

  GeometryTypeCtx(Ctx&& ctx)
    : Ctx(std::move(ctx))
  {}

  const Dune::GeometryType& geometry_type() const
  {
    return _gt;
  }

  void set_geometry_type(const Dune::GeometryType geometry_type)
  {
    _gt = geometry_type;
  }

  template<class BindCtx>
  std::enable_if_t<Concept::has_method_geometry_type<BindCtx>()>
  bind(const BindCtx& bind_ctx)
  {
    set_geometry_type(bind_ctx.geometry_type());
    Ctx::bind(bind_ctx);
  }

private:
  Dune::GeometryType _gt;
};

template<class Ctx>
struct GeometryTypeCtx<Ctx,std::enable_if_t<Concept::has_method_geometry_type<Ctx>()>> : public Ctx
{
  GeometryTypeCtx(Ctx&& ctx)
    : Ctx(std::move(ctx))
  {}
};

template<class Ctx>
auto inject_geometry_type(Ctx&& ctx, const Dune::GeometryType& geometry_type)
{
  GeometryTypeCtx<Ctx> gt_ctx{std::move(ctx)};
  gt_ctx.set_geometry_type(geometry_type);
  return gt_ctx;
}

auto make_geometry_type(const Dune::GeometryType& geometry_type)
{
  return inject_geometry_type(BaseCtx{},geometry_type);
}

} // namespace Dune::Copasi::Context

#endif // DUNE_COPASI_CONTEXT_GEOMETRY_TYPE_HH
