/*
 * Copyright 2018 Charles University in Prague
 * Copyright 2018 Vojtech Horky
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package cz.cuni.mff.d3s.perf;

import static org.junit.Assert.*;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

@RunWith(Parameterized.class)
public class PapiPresetEventsSupportTest {
	@Parameters(name = "{0}")
	public static Collection<Object[]> generateTestParameters() {
		/* cut '-d,' -f 2 src/papi_events.csv  | grep '^PAPI_' | sort | uniq | sed 's#.*#\t\t\t{ "PAPI:&" },#' */
		return Arrays.asList(new Object[][] {
			{ "PAPI:PAPI_BR_CN" },
			{ "PAPI:PAPI_BR_INS" },
			{ "PAPI:PAPI_BR_MSP" },
			{ "PAPI:PAPI_BR_NTK" },
			{ "PAPI:PAPI_BR_PRC" },
			{ "PAPI:PAPI_BR_TKN" },
			{ "PAPI:PAPI_BR_UCN" },
			{ "PAPI:PAPI_BTAC_M" },
			{ "PAPI:PAPI_CA_CLN" },
			{ "PAPI:PAPI_CA_INV" },
			{ "PAPI:PAPI_CA_ITV" },
			{ "PAPI:PAPI_CA_SHR" },
			{ "PAPI:PAPI_CA_SNP" },
			{ "PAPI:PAPI_CSR_FAL" },
			{ "PAPI:PAPI_CSR_SUC" },
			{ "PAPI:PAPI_CSR_TOT" },
			{ "PAPI:PAPI_DP_OPS" },
			{ "PAPI:PAPI_FAD_INS" },
			{ "PAPI:PAPI_FDV_INS" },
			{ "PAPI:PAPI_FMA_INS" },
			{ "PAPI:PAPI_FML_INS" },
			{ "PAPI:PAPI_FP_INS" },
			{ "PAPI:PAPI_FP_OPS" },
			{ "PAPI:PAPI_FP_STAL" },
			{ "PAPI:PAPI_FPU_IDL" },
			{ "PAPI:PAPI_FSQ_INS" },
			{ "PAPI:PAPI_FUL_CCY" },
			{ "PAPI:PAPI_FUL_ICY" },
			{ "PAPI:PAPI_FXU_IDL" },
			{ "PAPI:PAPI_HW_INT" },
			{ "PAPI:PAPI_INT_INS" },
			{ "PAPI:PAPI_LD_INS" },
			{ "PAPI:PAPI_LST_INS" },
			{ "PAPI:PAPI_L1_DCA" },
			{ "PAPI:PAPI_L1_DCM" },
			{ "PAPI:PAPI_L1_DCR" },
			{ "PAPI:PAPI_L1_DCW" },
			{ "PAPI:PAPI_L1_DCH" },
			{ "PAPI:PAPI_L1_ICA" },
			{ "PAPI:PAPI_L1_ICM" },
			{ "PAPI:PAPI_L1_ICR" },
			{ "PAPI:PAPI_L1_ICW" },
			{ "PAPI:PAPI_L1_ICH" },
			{ "PAPI:PAPI_L1_LDM" },
			{ "PAPI:PAPI_L1_STM" },
			{ "PAPI:PAPI_L1_TCA" },
			{ "PAPI:PAPI_L1_TCM" },
			{ "PAPI:PAPI_L1_TCR" },
			{ "PAPI:PAPI_L1_TCW" },
			{ "PAPI:PAPI_L1_TCH" },
			{ "PAPI:PAPI_L2_DCA" },
			{ "PAPI:PAPI_L2_DCM" },
			{ "PAPI:PAPI_L2_DCR" },
			{ "PAPI:PAPI_L2_DCW" },
			{ "PAPI:PAPI_L2_DCH" },
			{ "PAPI:PAPI_L2_ICA" },
			{ "PAPI:PAPI_L2_ICM" },
			{ "PAPI:PAPI_L2_ICR" },
			{ "PAPI:PAPI_L2_ICH" },
			{ "PAPI:PAPI_L2_LDH" },
			{ "PAPI:PAPI_L2_LDM" },
			{ "PAPI:PAPI_L2_STM" },
			{ "PAPI:PAPI_L2_TCA" },
			{ "PAPI:PAPI_L2_TCM" },
			{ "PAPI:PAPI_L2_TCR" },
			{ "PAPI:PAPI_L2_TCW" },
			{ "PAPI:PAPI_L2_TCH" },
			{ "PAPI:PAPI_L3_DCA" },
			{ "PAPI:PAPI_L3_DCM" },
			{ "PAPI:PAPI_L3_DCR" },
			{ "PAPI:PAPI_L3_DCW" },
			{ "PAPI:PAPI_L3_DCH" },
			{ "PAPI:PAPI_L3_ICA" },
			{ "PAPI:PAPI_L3_ICM" },
			{ "PAPI:PAPI_L3_ICR" },
			{ "PAPI:PAPI_L3_ICH" },
			{ "PAPI:PAPI_L3_LDH" },
			{ "PAPI:PAPI_L3_LDM" },
			{ "PAPI:PAPI_L3_STM" },
			{ "PAPI:PAPI_L3_TCA" },
			{ "PAPI:PAPI_L3_TCM" },
			{ "PAPI:PAPI_L3_TCR" },
			{ "PAPI:PAPI_L3_TCW" },
			{ "PAPI:PAPI_L3_TCH" },
			{ "PAPI:PAPI_MEM_RCY" },
			{ "PAPI:PAPI_MEM_SCY" },
			{ "PAPI:PAPI_MEM_WCY" },
			{ "PAPI:PAPI_PRF_DM" },
			{ "PAPI:PAPI_REF_CYC" },
			{ "PAPI:PAPI_RES_STL" },
			{ "PAPI:PAPI_SP_OPS" },
			{ "PAPI:PAPI_SR_INS" },
			{ "PAPI:PAPI_STL_CCY" },
			{ "PAPI:PAPI_STL_ICY" },
			{ "PAPI:PAPI_SYC_INS" },
			{ "PAPI:PAPI_TLB_DM" },
			{ "PAPI:PAPI_TLB_IM" },
			{ "PAPI:PAPI_TLB_SD" },
			{ "PAPI:PAPI_TLB_TL" },
			{ "PAPI:PAPI_TOT_CYC" },
			{ "PAPI:PAPI_TOT_IIS" },
			{ "PAPI:PAPI_TOT_INS" },
			{ "PAPI:PAPI_VEC_DP" },
			{ "PAPI:PAPI_VEC_INS" },
			{ "PAPI:PAPI_VEC_SP" },
		});
	}
	
	private final String eventName;
	private List<String> supportedEvents;
	
	public PapiPresetEventsSupportTest(String eventName) {
		this.eventName = eventName;
	}

	@Before
	public void readSupportedEvents() {
		supportedEvents = Measurement.getSupportedEvents();
	}

	@Test
	public void supportCorrespondsAcrossInterfaces() {
		boolean isSupported = Measurement.isEventSupported(eventName);
		boolean isInList = supportedEvents.contains(eventName);
		assertEquals(eventName, isSupported, isInList);
	}
}
