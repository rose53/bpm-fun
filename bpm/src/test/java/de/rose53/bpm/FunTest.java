package de.rose53.bpm;

import static com.github.tomakehurst.wiremock.client.WireMock.*;
import static com.github.tomakehurst.wiremock.core.WireMockConfiguration.wireMockConfig;
import static com.github.tomakehurst.wiremock.stubbing.Scenario.STARTED;
import static org.camunda.bpm.engine.test.assertions.bpmn.BpmnAwareTests.assertThat;
import static org.camunda.bpm.engine.test.assertions.bpmn.BpmnAwareTests.execute;
import static org.camunda.bpm.engine.test.assertions.bpmn.BpmnAwareTests.runtimeService;
import static org.camunda.bpm.engine.test.assertions.bpmn.BpmnAwareTests.job;

import com.github.tomakehurst.wiremock.common.ConsoleNotifier;
import com.github.tomakehurst.wiremock.junit.WireMockRule;
import org.camunda.bpm.engine.runtime.ProcessInstance;
import org.camunda.bpm.engine.test.Deployment;
import org.camunda.bpm.engine.test.ProcessEngineRule;
import org.junit.Rule;
import org.junit.Test;
import wiremock.org.eclipse.jetty.http.HttpStatus;


public class FunTest {

    @Rule
    public WireMockRule wireMockSoilRule = new WireMockRule(wireMockConfig().port(31415).notifier(new ConsoleNotifier(true)));

    @Rule
    public WireMockRule wireMockFlagRule = new WireMockRule(wireMockConfig().port(31416).notifier(new ConsoleNotifier(true)));

    @Rule
    public ProcessEngineRule rule = new ProcessEngineRule();

    @Test
    @Deployment(resources = {"funProcess.bpmn"})
    public void testIrrigation() {

        wireMockSoilRule.stubFor(get(urlEqualTo("/soilmoisture"))
                .inScenario("Irrigation Scenario")
                .whenScenarioStateIs(STARTED)
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)
                        .withHeader("Content-Type", "application/json")
                        .withBody("{ \"value\": \"99\", \"description\": \"irrigation\"}"))
                .willSetStateTo("Irrigation done"));

        wireMockSoilRule.stubFor(get(urlEqualTo("/soilmoisture"))
                .inScenario("Irrigation Scenario")
                .whenScenarioStateIs("Irrigation done")
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)
                        .withHeader("Content-Type", "application/json")
                        .withBody("{ \"value\": \"10\", \"description\": \"adequately wet\"}")));

        wireMockFlagRule.stubFor(put(urlEqualTo("/flag"))
                .withHeader("Content-Type", equalTo("application/json"))
                .withRequestBody(equalToJson("{ \"status\": false }"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)));

        wireMockSoilRule.stubFor(put(urlEqualTo("/pump"))
                .withHeader("Content-Type", equalTo("application/json"))
                .withRequestBody(equalToJson("{ \"status\": true }"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)));

        wireMockSoilRule.stubFor(put(urlEqualTo("/pump"))
                .withHeader("Content-Type", equalTo("application/json"))
                .withRequestBody(equalToJson("{ \"status\": false }"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)));

        // Given we create a new process instance
        ProcessInstance processInstance = runtimeService().startProcessInstanceByKey("funProcessId");

        execute(job(processInstance));

        // Then the process instance should be ended
        assertThat(processInstance).isEnded();
    }

    @Test
    @Deployment(resources = {"funProcess.bpmn"})
    public void testIrrigationAdvice() {

        wireMockSoilRule.stubFor(get(urlEqualTo("/soilmoisture"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)
                        .withHeader("Content-Type", "application/json")
                        .withBody("{ \"value\": \"50\", \"description\": \"irrigation advice\"}")));

        wireMockFlagRule.stubFor(put(urlEqualTo("/flag"))
                .withHeader("Content-Type", equalTo("application/json"))
                .withRequestBody(equalToJson("{ \"status\": true }"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)));

        // Given we create a new process instance
        ProcessInstance processInstance = runtimeService().startProcessInstanceByKey("funProcessId");

        // Then the process instance should be ended
        assertThat(processInstance).isEnded();
    }

    @Test
    @Deployment(resources = {"funProcess.bpmn"})
    public void testWet() {

        wireMockSoilRule.stubFor(get(urlEqualTo("/soilmoisture"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)
                        .withHeader("Content-Type", "application/json")
                        .withBody("{ \"value\": \"10\", \"description\": \"adequately wet\"}")));

        wireMockFlagRule.stubFor(put(urlEqualTo("/flag"))
                .withHeader("Content-Type", equalTo("application/json"))
                .withRequestBody(equalToJson("{ \"status\": false }"))
                .willReturn(aResponse().withStatus(HttpStatus.OK_200)));

        // Given we create a new process instance
        ProcessInstance processInstance = runtimeService().startProcessInstanceByKey("funProcessId");

        // Then the process instance should be ended
        assertThat(processInstance).isEnded();
    }
}
