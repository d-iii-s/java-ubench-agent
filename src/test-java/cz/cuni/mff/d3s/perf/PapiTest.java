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

import org.junit.*;

public class PapiTest {
	@Before
	public void papiWithExplicitPrefix() {
		Assume.assumeTrue(Measurement.isEventSupported("PAPI_TOT_INS"));
	}
	
	@Test
	public void withExplicitPrefix() {
		Assert.assertTrue(Measurement.isEventSupported("PAPI:PAPI_TOT_INS"));
	}
	
	@Test
	public void withComponentNamePrefix() {
		Assert.assertTrue(Measurement.isEventSupported("PAPI:perf::INSTRUCTIONS"));
	}
	
	@Test
	public void invalidEventName() {
		Assert.assertFalse(Measurement.isEventSupported("PAPI:perf::COMPLETE_NONSENSE_COUNTER"));	
	}
}
