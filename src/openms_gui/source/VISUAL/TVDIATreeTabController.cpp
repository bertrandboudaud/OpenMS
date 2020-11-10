// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2020.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/VISUAL/TVDIATreeTabController.h>

#include <OpenMS/CONCEPT/RAIICleanup.h>
#include <OpenMS/KERNEL/ChromatogramTools.h>
#include <OpenMS/KERNEL/OnDiscMSExperiment.h>
#include <OpenMS/VISUAL/APPLICATIONS/TOPPViewBase.h>
#include <OpenMS/VISUAL/AxisWidget.h>
#include <OpenMS/VISUAL/Plot1DWidget.h>

#include <QtWidgets/QMessageBox>
#include <QtCore/QString>

using namespace OpenMS;
using namespace std;

namespace OpenMS
{
  TVDIATreeTabController::TVDIATreeTabController(TOPPViewBase* parent):
    TVControllerBase(parent)
  {  
  }

  LayerData::ExperimentSharedPtrType prepareChromatogram(Size index, LayerData::ExperimentSharedPtrType exp_sptr, LayerData::ODExperimentSharedPtrType ondisc_sptr)
  {
    // create a managed pointer fill it with a spectrum containing the chromatographic data
    LayerData::ExperimentSharedPtrType chrom_exp_sptr(new LayerData::ExperimentType());
    chrom_exp_sptr->setMetaValue("is_chromatogram", "true"); //this is a hack to store that we have chromatogram data
    LayerData::ExperimentType::SpectrumType spectrum;

    // retrieve chromatogram (either from in-memory or on-disc representation)
    MSChromatogram current_chrom;
    current_chrom = exp_sptr->getChromatograms()[index];
    if (current_chrom.empty() )
    {
      current_chrom = ondisc_sptr->getChromatogram(index);
    }

    // fill "dummy" spectrum with chromatogram data
    for (Size i = 0; i != current_chrom.size(); ++i)
    {
      const ChromatogramPeak & cpeak = current_chrom[i];
      Peak1D peak1d;
      peak1d.setMZ(cpeak.getRT());
      peak1d.setIntensity(cpeak.getIntensity());
      spectrum.push_back(peak1d);
    }

    spectrum.getFloatDataArrays() = current_chrom.getFloatDataArrays();
    spectrum.getIntegerDataArrays() = current_chrom.getIntegerDataArrays();
    spectrum.getStringDataArrays() = current_chrom.getStringDataArrays();

    // Add at least one data point to the chromatogram, otherwise
    // "addLayer" will fail and a segfault occurs later
    if (current_chrom.empty()) 
    {
      Peak1D peak1d(-1, 0);
      spectrum.push_back(peak1d);
    }

    // store peptide_sequence if available
    if (current_chrom.getPrecursor().metaValueExists("peptide_sequence"))
    {
      chrom_exp_sptr->setMetaValue("peptide_sequence", current_chrom.getPrecursor().getMetaValue("peptide_sequence"));
    }

    chrom_exp_sptr->addSpectrum(spectrum);
    return chrom_exp_sptr;
  }

  void TVDIATreeTabController::showTransitionsAs1D(const std::vector<int>& indices)
  {
    // basic behavior 1

    // show multiple spectra together is only used for chromatograms directly
    // where multiple (SRM) traces are shown together
    LayerData & layer = const_cast<LayerData&>(tv_->getActiveCanvas()->getCurrentLayer());
    ExperimentSharedPtrType exp_sptr = layer.getPeakDataMuteable();
    auto ondisc_sptr = layer.getOnDiscPeakData();

    // string for naming the different chromatogram layers with their index
    String chromatogram_caption;
    // string for naming the tab title with the indices of the chromatograms
    String caption = layer.getName();

    //open new 1D widget
    Plot1DWidget * w = new Plot1DWidget(tv_->getSpectrumParameters(1), (QWidget *)tv_->getWorkspace());
    // fix legend if its a chromatogram
    w->xAxis()->setLegend(PlotWidget::RT_AXIS_TITLE);

    for (const auto& index : indices)
    {
      if (layer.type == LayerData::DT_CHROMATOGRAM)
      {
        ExperimentSharedPtrType chrom_exp_sptr = prepareChromatogram(index, exp_sptr, ondisc_sptr);

        // fix legend and set layer name
        caption += String(" [") + index + "];";
        chromatogram_caption = layer.getName() + "[" + index + "]";

        // add chromatogram data as peak spectrum
        if (!w->canvas()->addLayer(chrom_exp_sptr, ondisc_sptr, layer.filename))
        {
          return;
        }
        w->canvas()->setLayerName(w->canvas()->getCurrentLayerIndex(), chromatogram_caption);
        w->canvas()->setDrawMode(Plot1DCanvas::DM_CONNECTEDLINES);

        w->canvas()->getCurrentLayer().getChromatogramData() = exp_sptr; // save the original chromatogram data so that we can access it later

        //this is a hack to store that we have chromatogram data, that we selected multiple ones and which one we selected
        w->canvas()->getCurrentLayer().getPeakDataMuteable()->setMetaValue("is_chromatogram", "true");
        w->canvas()->getCurrentLayer().getPeakDataMuteable()->setMetaValue("multiple_select", "true");
        w->canvas()->getCurrentLayer().getPeakDataMuteable()->setMetaValue("selected_chromatogram", index);

        // set visible area to visible area in 2D view
        // switch X/Y because now we want to have RT on the x-axis and not m/z
        DRange<2> visible_area = tv_->getActiveCanvas()->getVisibleArea();
        int tmp_x1 = visible_area.minX();
        int tmp_x2 = visible_area.maxX();
        visible_area.setMinX(visible_area.minY());
        visible_area.setMaxX(visible_area.maxY());
        visible_area.setMinY(tmp_x1);
        visible_area.setMaxY(tmp_x2);
        w->canvas()->setVisibleArea(visible_area);
      }
    }

    // set relative (%) view of visible area
    w->canvas()->setIntensityMode(PlotCanvas::IM_SNAP);

    // basic behavior 2

    tv_->showPlotWidgetInWindow(w, caption);
    tv_->updateBarsAndMenus();
  }

  // called by SpectraTreeTab::chromsSelected()
  void TVDIATreeTabController::activate1DTransitions(const std::vector<int>& indices)
  {
    Plot1DWidget * widget_1d = tv_->getActive1DWidget();

    // return if no active 1D widget is present or no layers are present (e.g. the addLayer call failed)
    if (widget_1d == nullptr) return;
    if (widget_1d->canvas()->getLayerCount() == 0) return;

    const LayerData& layer = widget_1d->canvas()->getCurrentLayer();
    if (layer.getChromatogramAnnotation().get())
    {
      widget_1d->canvas()->removeLayers();
      widget_1d->canvas()->blockSignals(true);
      RAIICleanup clean([&]() {widget_1d->canvas()->blockSignals(false); });
      /*
      String fname = layer.filename;
      for (const auto& index : indices)
      {
        ExperimentSharedPtrType chrom_exp_sptr = prepareChromatogram(index, exp_sptr, ondisc_sptr);

        // get caption (either chromatogram idx or peptide sequence, if available)
        String caption = fname;
        if (chrom_exp_sptr->metaValueExists("peptide_sequence"))
        {
          caption = String(chrom_exp_sptr->getMetaValue("peptide_sequence"));
        }
        ((caption += "[") += index) += "]";
        // add chromatogram data as peak spectrum
        widget_1d->canvas()->addChromLayer(chrom_exp_sptr, ondisc_sptr, fname, caption, exp_sptr, index, true);
      }
           */
      tv_->updateBarsAndMenus(); // needed since we blocked update above (to avoid repeated layer updates for many layers!)
    }
  }

  void TVDIATreeTabController::deactivate1DTransitions(const std::vector<int>& /*indices*/)
  {
    // no special handling of spectrum deactivation needed
  }

} // OpenMS

