import numpy as np
import pyproj
import scipy.ndimage
from scipy import interpolate
from netCDF4 import Dataset
from Yoffe import regularizedYoffe


def writeNetcdf(sname, x, y, aName, aData, paraview_readable=False):
    """create a netcdf file either readable by ASAGI
    or by paraview (paraview_readable=True)"""
    if paraview_readable:
        fname = sname + "_paraview.nc"
    else:
        fname = sname + "_ASAGI.nc"
    print("writing " + fname)

    nx = x.shape[0]
    ny = y.shape[0]

    with Dataset(fname, "w", format="NETCDF4") as rootgrp:
        rootgrp.createDimension("u", nx)
        rootgrp.createDimension("v", ny)
        vx = rootgrp.createVariable("u", "f4", ("u",))
        vx[:] = x
        vy = rootgrp.createVariable("v", "f4", ("v",))
        vy[:] = y
        if paraview_readable:
            for i in range(len(aName)):
                vTd = rootgrp.createVariable(aName[i], "f4", ("v", "u"))
                vTd[:, :] = aData[i][:, :]
        else:
            ldata4 = [(name, "f4") for name in aName]
            ldata8 = [(name, "f8") for name in aName]
            mattype4 = np.dtype(ldata4)
            mattype8 = np.dtype(ldata8)
            mat_t = rootgrp.createCompoundType(mattype4, "material")

            # this transform the 4 D array into an array of tuples
            arr = np.stack([aData[i] for i in range(len(aName))], axis=2)
            newarr = arr.view(dtype=mattype8)
            newarr = newarr.reshape(newarr.shape[:-1])
            mat = rootgrp.createVariable("data", mat_t, ("v", "u"))
            mat[:] = newarr


def interpolate_nan_from_neighbors(array):
    """rise_time and tacc may not be defined where there is no slip (no SR function).
    in this case, we interpolate from neighbors
    source: https://stackoverflow.com/questions/37662180/interpolate-missing-values-2d-python
    """
    x = np.arange(0, array.shape[1])
    y = np.arange(0, array.shape[0])
    # mask invalid values
    array = np.ma.masked_invalid(array)
    xx, yy = np.meshgrid(x, y)
    # get only the valid values
    x1 = xx[~array.mask]
    y1 = yy[~array.mask]
    newarr = array[~array.mask]
    return interpolate.griddata((x1, y1), newarr.ravel(), (xx, yy), method="linear", fill_value=np.average(array))


def upsample_quantities(allarr, spatial_order, spatial_zoom, padding="constant", extra_padding_layer=False):
    """1. pad
    2. upsample, adding spatial_zoom per node
    """
    nd = allarr.shape[0]
    ny, nx = [val * spatial_zoom for val in allarr[0].shape]
    if extra_padding_layer:
        # required for vertex aligned netcdf format
        nx = nx + 2
        ny = ny + 2
    allarr0 = np.zeros((nd, ny, nx))
    for k in range(nd):
        if padding == "extrapolate":
            my_array = np.pad(allarr[k, :, :], ((1, 1), (1, 1)), "reflect", reflect_type="odd")
        else:
            my_array = np.pad(allarr[k, :, :], ((1, 1), (1, 1)), padding)
        if extra_padding_layer:
            ncrop = spatial_zoom - 1
        else:
            ncrop = spatial_zoom
        my_array = scipy.ndimage.zoom(my_array, spatial_zoom, order=spatial_order, mode="grid-constant", grid_mode=True)
        if ncrop > 0:
            allarr0[k, :, :] = my_array[ncrop:-ncrop, ncrop:-ncrop]

    return allarr0


class FaultPlane:
    def __init__(self):
        self.nx = 0
        self.ny = 0
        self.ndt = 0
        self.PSarea_cm2 = 0
        self.dt = 0
        # array member initialized to dummy value
        self.lon = 0
        self.lat = 0
        self.x = 0
        self.y = 0
        self.depth = 0
        self.t0 = 0
        self.slip1 = 0
        self.strike = 0
        self.dip = 0
        self.rake = 0
        self.aSR = 0
        self.myt = 0

    def init_spatial_arrays(self, nx, ny):
        self.nx = nx
        self.ny = ny
        self.lon = np.zeros((ny, nx))
        self.lat = np.zeros((ny, nx))
        self.x = np.zeros((ny, nx))
        self.y = np.zeros((ny, nx))
        self.depth = np.zeros((ny, nx))
        self.t0 = np.zeros((ny, nx))
        self.slip1 = np.zeros((ny, nx))
        self.strike = np.zeros((ny, nx))
        self.dip = np.zeros((ny, nx))
        self.rake = np.zeros((ny, nx))

    def init_aSR(self):
        self.aSR = np.zeros((self.ny, self.nx, self.ndt))

    def extend_aSR(self, ndt_old, ndt_new):
        "extend aSR array to more time samplings"
        tmpSR = np.copy(self.aSR)
        self.ndt = ndt_new
        self.aSR = np.zeros((self.ny, self.nx, self.ndt))
        self.aSR[:, :, 0:ndt_old] = tmpSR[:, :, :]

    def compute_xy_from_latlon(self, proj):
        if proj:
            from pyproj import Transformer

            transformer = Transformer.from_crs("epsg:4326", proj[0], always_xy=True)
            self.x, self.y = transformer.transform(self.lon, self.lat)
        else:
            print("no proj string specified!")
            self.x, self.y = self.lon, self.lat

    def compute_latlon_from_xy(self, proj):
        if proj:
            from pyproj import Transformer

            transformer = Transformer.from_crs(proj[0], "epsg:4326", always_xy=True)
            self.lon, self.lat = transformer.transform(self.x, self.y)
        else:
            self.lon, self.lat = self.x, self.y

    def compute_time_array(self):
        self.myt = np.linspace(0, (self.ndt - 1) * self.dt, self.ndt)

    def write_srf(self, fname):
        "write kinematic model to a srf file (standard rutpure format)"
        with open(fname, "w") as fout:
            fout.write("1.0\n")
            fout.write("POINTS %d\n" % (self.nx * self.ny))
            for j in range(self.ny):
                for i in range(self.nx):
                    fout.write("%g %g %g %g %g %e %g %g\n" % (self.lon[j, i], self.lat[j, i], self.depth[j, i], self.strike[j, i], self.dip[j, i], self.PSarea_cm2, self.t0[j, i], self.dt))
                    fout.write("%g %g %d %f %d %f %d\n" % (self.rake[j, i], self.slip1[j, i], self.ndt, 0.0, 0, 0.0, 0))
                    np.savetxt(fout, self.aSR[j, i, :], fmt="%g", newline=" ")
                    fout.write("\n")
        print("done writing", fname)

    def init_from_srf(self, fname):
        "init object by reading a srf file (standard rutpure format)"
        with open(fname) as fid:
            # version
            line = fid.readline()
            version = float(line)
            if not (abs(version - 1.0) < 1e-03 or abs(version - 2.0) < 1e-03):
                print("srf version: %s not supported" % (line))
                raise
            # skip comments
            while True:
                line = fid.readline()
                if not line.startswith("#"):
                    break
            line_el = line.split()
            if line_el[0] != "PLANE":
                print("no plane specified")
                raise
            if line_el[1] != "1":
                print("only one plane supported")
                raise NotImplementedError
            line_el = fid.readline().split()
            nx, ny = [int(val) for val in line_el[2:4]]
            line_el = fid.readline().split()
            # check that the plane data are consistent with the number of points
            assert int(line_el[1]) == nx * ny
            self.init_spatial_arrays(nx, ny)
            for j in range(ny):
                for i in range(nx):
                    # first header line
                    line = fid.readline()
                    # rho_vs are only present for srf version 2
                    self.lon[j, i], self.lat[j, i], self.depth[j, i], self.strike[j, i], self.dip[j, i], self.PSarea_cm2, self.t0[j, i], dt, *rho_vs = [float(v) for v in line.split()]
                    # second header line
                    line = fid.readline()
                    self.rake[j, i], self.slip1[j, i], ndt1, slip2, ndt2, slip3, ndt3 = [float(v) for v in line.split()]
                    if max(slip2, slip3) > 0.0:
                        print("this script assumes slip2 and slip3 are zero", slip2, slip3)
                        raise NotImplementedError
                    ndt1 = int(ndt1)
                    if max(i, j) == 0:
                        self.ndt = ndt1
                        self.dt = dt
                        self.init_aSR()
                    lSTF = []
                    if ndt1 == 0:
                        continue
                    if ndt1 > self.ndt:
                        print(f"a larger ndt ({ndt1}> {self.ndt}) was found for point source (i,j) = ({i}, {j}) extending aSR array...")
                        self.extend_aSR(self.ndt, ndt1)
                    if abs(dt - self.dt) > 1e-6:
                        print("this script assumes that dt is the same for all sources", dt, self.dt)
                        raise NotImplementedError
                    while True:
                        line = fid.readline()
                        lSTF.extend(line.split())
                        if len(lSTF) == ndt1:
                            self.aSR[j, i, 0:ndt1] = np.array([float(v) for v in lSTF])
                            break

    def assess_Yoffe_parameters(self):
        "compute rise_time (slip duration) and t_acc (peak SR) from SR time histories"
        self.rise_time = np.zeros((self.ny, self.nx))
        self.tacc = np.zeros((self.ny, self.nx))
        for j in range(self.ny):
            for i in range(self.nx):
                if not self.slip1[j, i]:
                    self.rise_time[j, i] = np.nan
                    self.tacc[j, i] = np.nan
                else:
                    first_non_zero = np.amin(np.where(self.aSR[j, i, :])[0])
                    last_non_zero = np.amax(np.where(self.aSR[j, i, :])[0])
                    id_max = np.where(self.aSR[j, i, :] == np.amax(self.aSR[j, i, :]))[0]
                    self.rise_time[j, i] = (last_non_zero - first_non_zero + 1) * self.dt
                    self.tacc[j, i] = (id_max - first_non_zero + 1) * self.dt
                    self.t0[j, i] += first_non_zero * self.dt
        self.rise_time = interpolate_nan_from_neighbors(self.rise_time)
        self.tacc = interpolate_nan_from_neighbors(self.tacc)

        print("slip rise_time (min, 50%, max)", np.amin(self.rise_time), np.median(self.rise_time), np.amax(self.rise_time))
        print("tacc (min, 50%, max)", np.amin(self.tacc), np.median(self.tacc), np.amax(self.tacc))

    def upsample_fault(self, spatial_order, spatial_zoom, temporal_zoom, proj, use_Yoffe=False):
        "increase spatial and temporal resolution of kinematic model by interpolation"
        # time vector
        ndt2 = (self.ndt - 1) * temporal_zoom + 1
        ny2, nx2 = self.ny * spatial_zoom, self.nx * spatial_zoom
        # resampled source
        pf = FaultPlane()
        pf.init_spatial_arrays(nx2, ny2)
        pf.ndt = ndt2
        pf.init_aSR()

        pf.dt = self.dt / temporal_zoom
        pf.compute_time_array()

        # upsample spatially geometry (bilinear interpolation)
        allarr = np.array([self.x, self.y, self.depth])
        pf.x, pf.y, pf.depth = upsample_quantities(allarr, spatial_order=1, spatial_zoom=spatial_zoom, padding="extrapolate")

        # upsample other quantities
        allarr = np.array([self.t0, self.strike, self.dip, self.rake])
        pf.t0, pf.strike, pf.dip, pf.rake = upsample_quantities(allarr, spatial_order, spatial_zoom, padding="edge")
        # the interpolation may generate some acausality that we here prevent
        pf.t0 = np.maximum(pf.t0, np.amin(self.t0))

        allarr = np.array([self.slip1])
        (pf.slip1,) = upsample_quantities(allarr, spatial_order, spatial_zoom, padding="constant")
        pf.compute_latlon_from_xy(proj)
        pf.PSarea_cm2 = self.PSarea_cm2 / spatial_zoom ** 2
        ratio_potency = np.sum(pf.slip1) * pf.PSarea_cm2 / (np.sum(self.slip1) * self.PSarea_cm2)
        print(f"potency ratio (upscaled over initial): {ratio_potency}")

        if use_Yoffe:
            self.assess_Yoffe_parameters()
            allarr = np.array([self.rise_time, self.tacc])
            pf.rise_time, pf.tacc = upsample_quantities(allarr, spatial_order, spatial_zoom, padding="edge")
            pf.rise_time = np.maximum(pf.rise_time, np.amin(self.rise_time))
            pf.tacc = np.maximum(pf.tacc, np.amin(self.tacc))
            ts = pf.tacc / 1.27
            tr = pf.rise_time - 2.0 * ts
            for j in range(pf.ny):
                for i in range(pf.nx):
                    for k, tk in enumerate(pf.myt):
                        pf.aSR[j, i, k] = pf.slip1[j, i] * regularizedYoffe(tk, ts[j, i], tr[j, i])
        else:
            aSRa = np.zeros((pf.ny, pf.nx, self.ndt))
            for k in range(self.ndt):
                aSRa[:, :, k] = upsample_quantities(np.array([self.aSR[:, :, k]]), spatial_order, spatial_zoom, padding="constant")

            # interpolate temporally the AST
            for j in range(pf.ny):
                for i in range(pf.nx):
                    f = interpolate.interp1d(self.myt, aSRa[j, i, :], kind="quadratic")
                    pf.aSR[j, i, :] = f(pf.myt)
                    # With a cubic interpolation, the interpolated slip1 may be negative which does not make sense.
                    if pf.slip1[j, i] < 0:
                        pf.aSR[j, i, :] = 0
                        continue
                    # should be the SR
                    integral_STF = np.trapz(np.abs(pf.aSR[j, i, :]), dx=pf.dt)
                    if abs(integral_STF) > 0:
                        pf.aSR[j, i, :] = pf.slip1[j, i] * pf.aSR[j, i, :] / integral_STF
        return pf

    def generate_netcdf_fl33(self, prefix, spatial_order, spatial_zoom, write_paraview):
        "generate netcdf files to be used with SeisSol friction law 33"

        cm2m = 0.01
        # a kinematic model defines the fault quantities at the subfault center
        # a netcdf file defines the quantities at the nodes
        # therefore the extra_padding_layer=True, and the added di below
        (slip,) = upsample_quantities(np.array([self.slip1]), spatial_order, spatial_zoom, padding="constant", extra_padding_layer=True)
        allarr = np.array([self.t0, self.rake, self.rise_time, self.tacc])
        rupttime, rake, rise_time, tacc = upsample_quantities(allarr, spatial_order, spatial_zoom, padding="edge", extra_padding_layer=True)
        # upsampled duration, rise_time and acc_time may not be smaller than initial values
        # at least rise_time could lead to a non-causal kinematic model
        rupttime = np.maximum(rupttime, np.amin(self.t0))
        rise_time = np.maximum(rise_time, np.amin(self.rise_time))
        tacc = np.maximum(tacc, np.amin(self.tacc))

        rake_rad = np.radians(rake)
        strike_slip = slip * np.cos(rake_rad) * cm2m
        dip_slip = slip * np.sin(rake_rad) * cm2m

        ny, nx = slip.shape
        dx = np.sqrt(self.PSarea_cm2 * cm2m * cm2m)
        ldataName = ["strike_slip", "dip_slip", "rupture_onset", "effective_rise_time", "acc_time"]
        lgridded_myData = [strike_slip, dip_slip, rupttime, rise_time, tacc]
        di = 1.0 / (2 * spatial_zoom)
        xb = np.linspace(-di, self.nx + di, nx) * dx
        yb = np.linspace(-di, self.ny + di, ny) * dx
        prefix2 = f"{prefix}_{spatial_zoom}_o{spatial_order}"
        if write_paraview:
            # see comment above
            for i, sdata in enumerate(ldataName):
                writeNetcdf(prefix2 + sdata, xb, yb, [sdata], [lgridded_myData[i]], paraview_readable=True)
        writeNetcdf(prefix2, xb, yb, ldataName, lgridded_myData)

    def generate_fault_yaml_fl33(self, prefix, spatial_order, spatial_zoom, proj):
        cm2m = 0.01
        km2m = 1e3
        self.compute_xy_from_latlon(proj)
        nx, ny = self.nx, self.ny
        p0 = np.array([self.x[0, 0], self.y[0, 0], -km2m * self.depth[0, 0]])
        p1 = np.array([self.x[ny - 1, 0], self.y[ny - 1, 0], -km2m * self.depth[ny - 1, 0]])
        p2 = np.array([self.x[0, nx - 1], self.y[0, nx - 1], -km2m * self.depth[0, nx - 1]])
        hw = p1 - p0
        dx1 = np.linalg.norm(hw) / ny
        hw = hw / np.linalg.norm(hw)
        hh = p2 - p0
        dx2 = np.linalg.norm(hh) / nx
        if abs(dx1 - dx2) / dx2 > 0.01:
            print("subfauts are not square", dx1, dx2)
            raise NotImplementedError
        hh = hh / np.linalg.norm(hh)
        dx = np.sqrt(self.PSarea_cm2 * cm2m * cm2m)
        # a kinematic model defines the fault quantities at the subfault center
        # a netcdf file defines the quantities at the nodes
        # therefore the dx/2
        t1 = -np.dot(p0, hh) + dx * 0.5
        t2 = -np.dot(p0, hw) + dx * 0.5
        template_yaml = f"""!Switch
[strike_slip, dip_slip, rupture_onset, effective_rise_time, acc_time]: !EvalModel
    parameters: [strike_slip, dip_slip, rupture_onset, effective_rise_time, acc_time]
    model: !Switch
        [strike_slip, dip_slip, rupture_onset, effective_rise_time, acc_time]: !AffineMap
              matrix:
                ua: [{hh[0]}, {hh[1]}, {hh[2]}]
                ub: [{hw[0]}, {hw[1]}, {hw[2]}]
              translation:
                ua: {t1}
                ub: {t2}
              components: !Any
                - !ASAGI
                    file: {prefix}_{spatial_zoom}_o{spatial_order}_ASAGI.nc
                    parameters: [strike_slip, dip_slip, rupture_onset, effective_rise_time, acc_time]
                    var: data
                    interpolation: linear
                - !ConstantMap
                  map:
                    strike_slip: 0.0
                    dip_slip:    0.0
                    rupture_onset:    0.0
                    acc_time:  1e100
                    effective_rise_time:  1e100
    components: !FunctionMap
       map:
          #Note the minus on strike_slip to acknowledge the different convention of SeisSol (T_s>0 means right-lateral)
          strike_slip: return -strike_slip;
          dip_slip: return dip_slip;
          rupture_onset: return rupture_onset;
          acc_time: return acc_time;
          effective_rise_time: return effective_rise_time;
        """
        fname = f"{prefix}_fault.yaml"
        with open(fname, "w") as fid:
            fid.write(template_yaml)
        print(f"done writing {fname}")
