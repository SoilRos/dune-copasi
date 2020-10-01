---
id: param_tree
title: Parameter Tree
sidebar_label: Parameter Tree
hide_table_of_contents: false
---

:::caution Work In Progress
:::

## Reference Keys

The parameter tree is a collection of *key–value pairs* that may be grouped by
sections. We call it a *tree* because sections may contain also other sections.
Here, we follow the [`DUNE` ini file convention](ini_file.md) to refer to
sections and keyords.

### `[grid]`

The grid should be formed by triangles and recangles in 2D, tetraedra and
hexahedra in 3D. Each *GMSH physical group* can only be formed by one of these
geometric types.

:::info Not required when using API
This section is only necessary on the executables. The grid may be constructed
by other forms other than with GMSH when using API version.
:::

:::caution Bug in GMSH reader
The GMSH file should not contain the `$PhysicalGroup` section at the
begining of the file (this is a knwon bug in the `dune-grid` GMSH reader).
:::

| Key | Description |
| ----------------| -------------- |
| `dimension`     | The dimension of the grid |
| `file`          | [GMSH v2 file][GMSH] directory absolute or relative to the executable |
| `initial_level` | The refinement level to start the simulation with |


### `[model]`

This section is the one that contains most of the aspects for simulation.

| Key | Description |
| ----------------| -------------- |
| `order`           | Finite Element polynomial order. if `order==0` FV else CG |
| `[time_stepping]` | Options for time-stepping |
| `[compartment]`   | List of compartmentes to simulate |
| `[<compartment>]` | Options for equations per each compartment |

:::info Polynomail Order
Whenever a compartment is coposed by cubes (2D) or hexahedra (3D), then such
compartment is modeled using Finite Volumes. Therefore, these compartments are
ideal to model membranes processes.
:::


#### `[model.time_stepping]`

This section sets the settings for the evolution of the forward model.

| Key | Description |
| ----------------| -------------- |
| `rk_method`  | Runge-Kutta method |
| `begin` | Initial time of the forward simulation |
| `end`   | Final time of the forward simulation |
| `initial_step` | Time step at the beginning of the simulation |
| `min_step` | Maxumum time step allowed |
| `max_step` | Minimum time step allowed |
| `decrease_factor` | Time step decrease factor when time step fails |
| `increase_factor` | Time step increase factor when time step succeds |

#### `[model.compartment]`

Each compartment corresponds to a *physical group* in the gmsh identifiers.
Although the gmsh format allows you to name such physical groups,
we still need to assign them into a `DuneCopasi` compartmet.
Notice that `dune-copasi` uses 0-based
 indices while `gmsh` uses 1-based indices. In other words,
`<gmsh_physical_group> = <dune_copasi_compartment> + 1`.
Let's say for example that there is two *physical groups* in our gmsh file
and we are going to use them as `nucleous` and `cytoplasm` compartments:

```ini
[model.compartments]
 # nucleous corresponds to the physical group 1 in the gmsh file
nucleous  = 0
 # cytoplasm corresponds to the physical group 2 in the gmsh file
cytoplasm = 0
```

#### `[model.<compartment>]`

Now, each of these compartments will define its own initial conditions,
its diffusion-reaction system, and its vtk writter. For that, you have to expand
the `model` section with the defined compartments, e.g. `model.nucleous` or `model.cytoplasm`.
The subsection `initial`, `reaction`, `diffusion` and `operator` define the system variables
and its properties. You can put as many variables as desired as long as they are the same
in this three subsections. Notice that each variable defines a new diffusion-reaction partial
differential equation associated with it.

##### `model.<compartment>.initial`
  * The `initial` subsection allow the initialization of each of the variables.
##### `model.<compartment>.diffusion`
  * The `diffusion` subsection defines the diffusion coefficient math
  expression associated with each variable. It may only depend on the grid coordinates `x` and `y`.
##### `model.<compartment>.reaction`
  * The `reaction` subsection defines the reaction part of each equation in the PDE.
  Since this is the souce of non-linearities, it allows to be dependent on other defined variables
  within the compartment. This section has to include yet another subsection called `jacobian`.
###### `model.<compartment>.reaction.jacobian`
  * The `reaction.jacobian` subsection must describe the
  [jacobian matrix](https://en.wikipedia.org/wiki/Jacobian_matrix_and_determinant)
  of the `reaction` part. It must follow the syntax of `d<var_i>_d<var_j>`, which
  reads as the *partial derivate of `<var_i>` with respect to `<var_j>`*.

For example, the following `mode.nucleous` section defines a [Gray-Scott
model with `F=0.0420` and `k=0.0610`](http://mrob.com/pub/comp/xmorphia/F420/F420-k610.html):



```ini
[model.nucleous.initial]
u = 0.5
v = (x>0) && (x<0.5) && (y>0.) && (y<0.5) ? 1 : 0

[model.nucleous.diffusion]
u = 2e-5
v = 2e-5/2

[model.nucleous.reaction]
u = -u*v^2+0.0420*(1-u)
v = u*v^2-(0.0420+0.0610)*v

[model.nucleous.reaction.jacobian]
du_du = -v^2-0.0420
du_dv = -2*u*v
dv_du = v^2
dv_dv = 2*u*v-(0.0420+0.0610)

[model.nucleous.operator]
u = 0
v = 0

[model.nucleous.writer]
file_name = nucleous_output
```
The `model.cytoplasm` would have to be defined in similar way. An important aspect when working
with different compartments is the interface fluxes. In `dune-copasi` thex fluxes are set
automatically to [dirichlet-dirichlet](https://en.wikipedia.org/wiki/Dirichlet_boundary_condition)
boundary conditions iff the variable is shared between the two intersecting domains. Further
improvements will come for interface treatment.

### `[logging]`

The logging settings are directly forwarded to the `dune-logging` module. Please check
its doxygen documentation for detailed information. A simple configuration is the following:

```ini
[logging]
# possible levels: off, critical, error, waring, notice, info, debug, trace, all
default.level = trace

[logging.sinks.stdout]
pattern = [{reldays:0>2}-{reltime:8%T}] [{backend}] {msg}
```

## Runge-Kutta methods

|  Method |
| ----------------------- |
| `explicit_euler`        |
| `implicit_euler`        |
| `heun`                  |
| `shu_3`                 |
| `runge_kutta_4`         |
| `alexander_2`           |
| `fractional_step_theta` |
| `alexander_3`           |



## Example

```ini
[grid]
dimension = 2
file = simple_grid.msh
initial_level = 1

[model]
order = 1

[model.time_stepping]
rk_method = alexander_2
begin = 0
end = 1
initial_step = 0.3
min_step = 1e-3
max_step = 0.35
decrease_factor = 0.5
increase_factor = 1.5

[model.compartments]
domain = 0

[model.domain.initial]
u = 0.5
v = (x>0) && (x<0.5) && (y>0.) && (y<0.5) ? 1 : 0

[model.domain.diffusion]
u = 2e-5
v = 2e-5/2

[model.domain.reaction]
u = -u*v^2+0.0420*(1-u)
v = u*v^2-(0.0420+0.0610)*v

[model.domain.reaction.jacobian]
du_du = -v^2-0.0420
du_dv = -2*u*v
dv_du = v^2
dv_dv = 2*u*v-(0.0420+0.0610)

[model.domain.operator]
u = 0
v = 0

[model.writer]
file_path = gray_scott
```

[GMSH]: http://gmsh.info/
